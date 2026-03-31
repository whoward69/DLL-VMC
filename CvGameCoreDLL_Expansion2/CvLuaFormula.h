#pragma once
#include "CvEnums.h"
#include "sstream"

class CvLuaFormula : public CvBaseInfo
{
public:
    bool CacheResults(Database::Results& kResults, CvDatabaseUtility& kUtility) override;
    std::string GetFormula() const;

private:
    std::string m_strFormula;
};

class CvLuaFormulaXMLEntries {
public:
    std::vector<CvLuaFormula*>& GetEntries();
    CvLuaFormula* GetEntry(LuaFormulaTypes eFormula);
    int GetNumEntries();
private:
    std::vector<CvLuaFormula*> m_vEntries;
};

class CvGlobals;

namespace lua {
    template <typename T>
    struct Result {
        bool ok = true;
        T value;

        static Result Error()
        {
            Result result;
            result.ok = false;
            return result;
        }

        static Result Ok(T value)
        {
            Result result;
            result.ok = true;
            result.value = value;
            return result;
        }
    };

    class EvaluatorFactory;

    template <typename FirstArg, typename... OtherArgs>
    inline void Dump(std::ostringstream& out_str, FirstArg first, OtherArgs... args)
    {
        Dump(out_str, first);
        Dump(out_str, args...);
    }

    template <typename Arg>
    inline void Dump(std::ostringstream& out_str, Arg first)
    {
        out_str << first << ",";
    }

    template <typename FirstArg, typename... OtherArgs>
    inline void Push(lua_State* l, FirstArg first, OtherArgs... args)
    {
        Push(l, first);
        Push(l, args...);
    }

    inline void Push(lua_State* l, int value)
    {
        lua_pushinteger(l, value);
    }

    inline void Push(lua_State* l, double value)
    {
        lua_pushnumber(l, value);
    }

    inline void Push(lua_State* l, bool value)
    {
        lua_pushboolean(l, value);
    }

    inline bool Pop(lua_State* l, int* out_value)
    {
        if (!lua_isnumber(l, -1))
        {
            lua_pop(l, 1);
            return false;
        }

        if (out_value)
        {
            *out_value = lua_tointeger(l, -1);
        }
        lua_pop(l, 1);
        return true;
    }

    inline bool Pop(lua_State* l, double* out_value)
    {
        if (!lua_isnumber(l, -1))
        {
            lua_pop(l, 1);
            return false;
        }

        if (out_value)
        {
            *out_value = lua_tonumber(l, -1);
        }
        lua_pop(l, 1);
        return true;
    }

    inline bool Pop(lua_State* l, bool* out_value)
    {
        if (!lua_isboolean(l, -1))
        {
            lua_pop(l, 1);
            return false;
        }

        if (out_value)
        {
            *out_value = lua_toboolean(l, -1);
        }
        lua_pop(l, 1);
        return true;
    }

    class Evaluator {
    private:
        friend class EvaluatorFactory;

        void LogMsg(const char* format, ...)
        {
            const size_t kBuffSize = 1024;
            static char buf[kBuffSize];
            const uint uiFlags = 0;    // Default (0) is to not write to console and to time stamp

            va_list vl;
            va_start(vl, format);
            vsprintf_s(buf, format, vl);
            va_end(vl);

            LOGFILEMGR.GetLog("embedding_lua.log", uiFlags)->Msg(buf);
        }

#undef DEBUG_EVALUATOR // can activate if you want to check if the stack leak.
#ifdef DEBUG_EVALUATOR
        struct StackChecker {
            Evaluator* parent;
            StackChecker(Evaluator* parent) : parent(parent) {}
            ~StackChecker()
            {
                if (parent == nullptr) return;

                int n = lua_gettop(parent->m_pL);
                parent->LogMsg("DEBUG %s: stack_size=%d", parent->m_pLuaFormula->GetType(), n);
            }
        };

        friend class StackChecker;
#endif

    public:
        template <typename T, typename... Args>
        Result<T> Evaluate(Args... args)
        {
#ifdef DEBUG_EVALUATOR
            StackChecker check{ this };
#endif

            if (m_pL == nullptr || m_pLuaFormula == nullptr)
            {
                return Result<T>::Error();
            }

            lua_getglobal(m_pL, m_strGlobalName.c_str());
            if (!lua_isfunction(m_pL, -1))
            {
                LogMsg("%s: cannot find global function", m_strGlobalName.c_str());
                lua_pop(m_pL, 1);
                return Result<T>::Error();
            }

            Push(m_pL, args...);

            int code = lua_pcall(m_pL, sizeof...(Args), 1, 0); // only support single return value now
            if (code != 0)
            {
                const char* err_msg = lua_tostring(m_pL, -1);
                std::ostringstream ss;
                Dump<Args...>(ss, args...);
                LogMsg("%s: pcall failed. args=%s; formula=%s; msg=%s", m_pLuaFormula->GetType(), ss.str().c_str(), m_pLuaFormula->GetFormula().c_str(), err_msg != nullptr ? err_msg : "null msg");
                lua_pop(m_pL, 1);
                return Result<T>::Error();
            }

            T result = 0;
            if (!Pop(m_pL, &result))
            {
                LogMsg("%s: pop failed", m_pLuaFormula->GetType());
            }
            return Result<T>::Ok(result);
        }

        Evaluator() = default;
        ~Evaluator()
        {
            if (m_pL != nullptr)
            {
                lua_close(m_pL);
                m_pL = nullptr;
            }
        }

        Evaluator& operator=(Evaluator&& other)
        {
            if (&other == this)
            {
                return *this;
            }

            this->m_pL = other.m_pL;
            this->m_pLuaFormula = other.m_pLuaFormula;
            this->m_strGlobalName = other.m_strGlobalName;

            other.m_pL = nullptr;
            other.m_pLuaFormula = nullptr;
            other.m_strGlobalName = "";

            return *this;
        }

        Evaluator& operator=(const Evaluator& other) = delete;

    private:
        CvLuaFormula* m_pLuaFormula = nullptr; // weak pointer;
        lua_State* m_pL = nullptr;
        std::string m_strGlobalName = "";
    };

    class EvaluatorFactory
    {
    public:
        bool Init(CvLuaFormula* pFormula, Evaluator* outEvaluator)
        {
            if (pFormula == nullptr || outEvaluator == nullptr)
            {
                return false;
            }

            lua_State* l = luaL_newstate();
            luaL_openlibs(l);

            int code = luaL_loadstring(l, pFormula->GetFormula().c_str());
            if (code != 0)
            {
                const char* err_msg = lua_tostring(l, -1);
                outEvaluator->LogMsg("%s: loadstring failed. formula=%s; err_msg=%s", pFormula->GetType(), pFormula->GetFormula().c_str(), err_msg);
                lua_pop(l, 1);
                lua_close(l);
                return false;
            }
            if (!lua_isfunction(l, -1))
            {
                outEvaluator->LogMsg("%s: init failed. top is not a function. formula=%s", pFormula->GetType(), pFormula->GetFormula().c_str());
                lua_pop(l, 1);
                lua_close(l);
                return false;
            }
            outEvaluator->m_strGlobalName = "func";
            outEvaluator->m_pLuaFormula = pFormula;
            outEvaluator->m_pL = l;
            lua_setglobal(l, outEvaluator->m_strGlobalName.c_str());
            outEvaluator->LogMsg("%s: init successfully. formula=%s", pFormula->GetType(), pFormula->GetFormula().c_str());
            return true;
        }
    };

    class EvaluatorManager {
    public:
        Evaluator* GetEvaluator(const char* type);
        Evaluator* GetEvaluator(LuaFormulaTypes eType);

        bool Init(CvGlobals* gc);
    private:
        CvGlobals* m_pGlobal = nullptr; // weak pointer
        std::tr1::unordered_map<LuaFormulaTypes, Evaluator> m_data;
    };
}
