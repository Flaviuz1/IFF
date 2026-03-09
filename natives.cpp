#include "natives.hpp"
#include <iostream>
#include <cmath>
#include <ctime>
#include <cfloat>
#include <experimental/random>
#include <cstring>

// @section: IO
Value clockNative(int argCount, Value* args) {
    return NUMBER_VAL((double)clock() / CLOCKS_PER_SEC);
}

Value printNative(int argCount, Value* args) {
    printValue(args[0]);
    printf("\n");
    return NULL_VAL;
}

Value printnNative(int argCount, Value* args) {
    printValue(args[0]);
    return NULL_VAL;
}

Value inputNative(int argCount, Value* args) {
    if (argCount > 0) {
        printValue(args[0]);  // optional prompt
    }
    std::string line;
    std::getline(std::cin, line);
    return STRING_VAL(copyString(line.c_str(), line.length()));
}
// @endsection

// @section: Math
Value absNative(int argCount, Value* args) {
    return NUMBER_VAL(fabs(AS_NUMBER(args[0])));
}

Value maxNative(int argCount, Value* args) {
    double MAX = DBL_MIN;
    for(int i = 0; i < argCount; i++) {
        double num = AS_NUMBER(args[i]);
        if (MAX < num) MAX = num;
    }
    return NUMBER_VAL(MAX);
}

Value minNative(int argCount, Value* args) {
    double MIN = DBL_MAX;
    for(int i = 0; i < argCount; i++) {
        double num = AS_NUMBER(args[i]);
        if (MIN > num) MIN = num;
    }
    return NUMBER_VAL(MIN);
}

Value floorNative(int argCount, Value* args) {
    return NUMBER_VAL(floor(AS_NUMBER(args[0])));
}

Value ceilNative(int argCount, Value* args) {
    return NUMBER_VAL(ceil(AS_NUMBER(args[0])));
}

Value roundNative(int argCount, Value* args) {
    return NUMBER_VAL(round(AS_NUMBER(args[0])));
}

Value truncNative(int argCount, Value* args) {
    return NUMBER_VAL(trunc(AS_NUMBER(args[0])));
}

Value signNative(int argCount, Value* args) {
    double n = AS_NUMBER(args[0]);
    return NUMBER_VAL((double)(n > 0 ? 1 : n < 0 ? -1 : 0));
}

Value fractNative(int argCount, Value* args) {
    double n = AS_NUMBER(args[0]);
    return NUMBER_VAL(n - floor(n));
}

Value clampNative(int argCount, Value* args) {
    double val = AS_NUMBER(args[0]);
    double mn  = AS_NUMBER(args[1]);
    double mx  = AS_NUMBER(args[2]);
    return NUMBER_VAL(val < mn ? mn : val > mx ? mx : val);
}

Value lerpNative(int argCount, Value* args) {
    double a = AS_NUMBER(args[0]);
    double b = AS_NUMBER(args[1]);
    double t = AS_NUMBER(args[2]);
    return NUMBER_VAL(a + t * (b - a));
}

Value expNative(int argCount, Value* args) {
    return NUMBER_VAL(exp(AS_NUMBER(args[0])));
}

Value logNative(int argCount, Value* args) {
    return NUMBER_VAL(log(AS_NUMBER(args[0])));
}

Value log2Native(int argCount, Value* args) {
    return NUMBER_VAL(log2(AS_NUMBER(args[0])));
}

Value log10Native(int argCount, Value* args) {
    return NUMBER_VAL(log10(AS_NUMBER(args[0])));
}

Value sinNative(int argCount, Value* args) {
    return NUMBER_VAL(sin(AS_NUMBER(args[0])));
}

Value cosNative(int argCount, Value* args) {
    return NUMBER_VAL(cos(AS_NUMBER(args[0])));
}

Value tanNative(int argCount, Value* args) {
    return NUMBER_VAL(tan(AS_NUMBER(args[0])));
}

Value asinNative(int argCount, Value* args) {
    return NUMBER_VAL(asin(AS_NUMBER(args[0])));
}

Value acosNative(int argCount, Value* args) {
    return NUMBER_VAL(acos(AS_NUMBER(args[0])));
}

Value atanNative(int argCount, Value* args) {
    return NUMBER_VAL(atan(AS_NUMBER(args[0])));
}

Value atan2Native(int argCount, Value* args) {
    return NUMBER_VAL(atan2(AS_NUMBER(args[0]), AS_NUMBER(args[1])));
}
// @endsection

// @section: Random 
Value randomNative (int argCount, Value* args) {
    return NUMBER_VAL((double)rand() / RAND_MAX);
}

Value seedNative(int argCount, Value* args) {
    srand((unsigned int)AS_NUMBER(args[0]));
    return NULL_VAL;
}

Value randominNative(int argCount, Value* args) {
    int u = AS_NUMBER(args[0]), d = AS_NUMBER(args[1]);
    if (u > d) {int a = u; u = d; d = a;}
    return NUMBER_VAL((double)std::experimental::randint(u, d));
}
// @endsection
//arrays

// @section: String
Value lenNative(int argCount, Value* args) {
    return NUMBER_VAL((double)(AS_STRING(args[0])->stringValue.length()));
}

Value upperNative(int argCount, Value* args) {
    std::string s = AS_STRING(args[0])->stringValue;
    for (char& c : s) c = toupper(c);
    return STRING_VAL(copyString(s.c_str(), s.length()));
}

Value lowerNative(int argCount, Value* args) {
    std::string s = AS_STRING(args[0])->stringValue;
    for (char& c : s) c = tolower(c);
    return STRING_VAL(copyString(s.c_str(), s.length()));
}

Value trimNative(int argCount, Value* args) {
    std::string s = AS_STRING(args[0])->stringValue;
    size_t start = s.find_first_not_of(" \t\r\n");
    size_t end   = s.find_last_not_of(" \t\r\n");
    if (start == std::string::npos) return STRING_VAL(copyString("", 0));
    s = s.substr(start, end - start + 1);
    return STRING_VAL(copyString(s.c_str(), s.length()));
}

Value substrNative(int argCount, Value* args) {
    std::string s   = AS_STRING(args[0])->stringValue;
    int start       = (int)AS_NUMBER(args[1]);
    int len         = (int)AS_NUMBER(args[2]);
    std::string res = s.substr(start, len);
    return STRING_VAL(copyString(res.c_str(), res.length()));
}

Value containsNative(int argCount, Value* args) {
    std::string s   = AS_STRING(args[0])->stringValue;
    std::string sub = AS_STRING(args[1])->stringValue;
    return BOOL_VAL(s.find(sub) != std::string::npos);
}

Value indexOfNative(int argCount, Value* args) {
    std::string s   = AS_STRING(args[0])->stringValue;
    std::string sub = AS_STRING(args[1])->stringValue;
    size_t pos = s.find(sub);
    return NUMBER_VAL(pos == std::string::npos ? -1 : (double)pos);
}

Value charAtNative(int argCount, Value* args) {
    std::string s = AS_STRING(args[0])->stringValue;
    int idx       = (int)AS_NUMBER(args[1]);
    char c        = s[idx];
    return STRING_VAL(copyString(&c, 1));
}

Value charCodeNative(int argCount, Value* args) {
    std::string s = AS_STRING(args[0])->stringValue;
    int idx       = (int)AS_NUMBER(args[1]);
    return NUMBER_VAL((double)s[idx]);
}

Value fromCharCodeNative(int argCount, Value* args) {
    char c = (char)AS_NUMBER(args[0]);
    return STRING_VAL(copyString(&c, 1));
}

Value replaceNative(int argCount, Value* args) {
    std::string s    = AS_STRING(args[0])->stringValue;
    std::string from = AS_STRING(args[1])->stringValue;
    std::string to   = AS_STRING(args[2])->stringValue;
    size_t pos = 0;
    while ((pos = s.find(from, pos)) != std::string::npos) {
        s.replace(pos, from.length(), to);
        pos += to.length();
    }
    return STRING_VAL(copyString(s.c_str(), s.length()));
}

Value repeatNative(int argCount, Value* args) {
    std::string s = AS_STRING(args[0])->stringValue;
    int n         = (int)AS_NUMBER(args[1]);
    std::string res;
    res.reserve(s.length() * n);
    for (int i = 0; i < n; i++) res += s;
    return STRING_VAL(copyString(res.c_str(), res.length()));
}

Value startsWithNative(int argCount, Value* args) {
    std::string s   = AS_STRING(args[0])->stringValue;
    std::string pre = AS_STRING(args[1])->stringValue;
    return BOOL_VAL(s.rfind(pre, 0) == 0);
}

Value endsWithNative(int argCount, Value* args) {
    std::string s   = AS_STRING(args[0])->stringValue;
    std::string suf = AS_STRING(args[1])->stringValue;
    if (suf.length() > s.length()) return BOOL_VAL(false);
    return BOOL_VAL(s.compare(s.length() - suf.length(), suf.length(), suf) == 0);
}
// @endsection

// @section: Conversions
    Value toNumberNative(int argCount, Value* args) {
        Value v = args[0];
        if (IS_NUMBER(v)) return v;
        if (IS_BOOL(v))   return NUMBER_VAL((double)(AS_BOOL(v) ? 1 : 0));
        if (IS_NULL(v))   return NUMBER_VAL(0);
        if (IS_STRING(v)) {
            try {
                double d = std::stod(AS_STRING(v)->stringValue);
                return NUMBER_VAL(d);
            } catch (...) {
                return NUMBER_VAL(NAN);
            }
        }
        return NUMBER_VAL(NAN);
    }

    Value toStringNative(int argCount, Value* args) {
        Value v = args[0];
        if (IS_STRING(v)) return v;
        if (IS_NUMBER(v)) {
            std::string s = std::to_string(AS_NUMBER(v));
            // trim trailing zeros
            s.erase(s.find_last_not_of('0') + 1);
            if (s.back() == '.') s.pop_back();
            return STRING_VAL(copyString(s.c_str(), s.length()));
        }
        if (IS_BOOL(v)) {
            const char* s = AS_BOOL(v) ? "true" : "false";
            return STRING_VAL(copyString(s, strlen(s)));
        }
        if (IS_NULL(v)) return STRING_VAL(copyString("null", 4));
        return STRING_VAL(copyString("unknown", 7));
    }

    Value toBoolNative(int argCount, Value* args) {
        Value v = args[0];
        if (IS_BOOL(v))   return v;
        if (IS_NULL(v))   return BOOL_VAL(false);
        if (IS_NUMBER(v)) return BOOL_VAL(AS_NUMBER(v) != 0);
        if (IS_STRING(v)) return BOOL_VAL(AS_STRING(v)->stringValue.length() > 0);
        return BOOL_VAL(false);
    }

    Value typeOfNative(int argCount, Value* args) {
        Value v = args[0];
        if (IS_BOOL(v))   return STRING_VAL(copyString("bool",     4));
        if (IS_NULL(v))   return STRING_VAL(copyString("null",     4));
        if (IS_NUMBER(v)) return STRING_VAL(copyString("number",   6));
        if (IS_STRING(v)) return STRING_VAL(copyString("string",   6));
        if (IS_OBJ(v) && OBJ_TYPE(v) == OBJ_FUNCTION) return STRING_VAL(copyString("function", 8));
        if (IS_OBJ(v) && OBJ_TYPE(v) == OBJ_NATIVE)   return STRING_VAL(copyString("function", 8));
        return STRING_VAL(copyString("unknown", 7));
    }

    Value isNumberNative(int argCount, Value* args) { return BOOL_VAL(IS_NUMBER(args[0])); }
    Value isStringNative(int argCount, Value* args) { return BOOL_VAL(IS_STRING(args[0])); }
    Value isBoolNative  (int argCount, Value* args) { return BOOL_VAL(IS_BOOL  (args[0])); }
    Value isNullNative  (int argCount, Value* args) { return BOOL_VAL(IS_NULL  (args[0])); }
    Value isFuncNative  (int argCount, Value* args) {
        return BOOL_VAL(IS_OBJ(args[0]) && 
            (OBJ_TYPE(args[0]) == OBJ_FUNCTION || OBJ_TYPE(args[0]) == OBJ_NATIVE));
    }
    Value isNaNNative   (int argCount, Value* args) { return BOOL_VAL(IS_NUMBER(args[0]) && std::isnan(AS_NUMBER(args[0]))); }
    Value isInfNative   (int argCount, Value* args) { return BOOL_VAL(IS_NUMBER(args[0]) && std::isinf(AS_NUMBER(args[0]))); }
// @endsection