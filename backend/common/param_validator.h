// common/param_validator.h
#pragma once

#include <drogon/HttpRequest.h>
#include <json/json.h>

#include <charconv>
#include <cstdint>
#include <string>
#include <utility>

#include "common/errors.h"
#include "utils/utils.h"

namespace
{

/// 可选字段缺失则为默认值
/// 数值字段默认为0 字符串默认为空 条件默认为false

/// 从请求体获取参数并校验
class BodyFieldValidator
{
public:
    BodyFieldValidator(const Json::Value& root, std::string name)
        : m_root(root), m_name(std::move(name))
    {
    }

    // ---------------- 存在性 + 类型 ----------------

    BodyFieldValidator& optional()
    {
        m_optional = true;
        m_present  = m_root.isMember(m_name);
        return *this;
    }

    BodyFieldValidator& require_string()
    {
        if (skip())
            return *this;

        if (!m_root.isMember(m_name))
            fail("is required");
        if (!m_root[m_name].isString())
            fail("must be a string");
        m_val  = m_root[m_name].asString();
        m_type = Type::String;
        return *this;
    }

    BodyFieldValidator& require_int()
    {
        if (skip())
            return *this;

        if (!m_root.isMember(m_name))
            fail("is required");
        if (!m_root[m_name].isInt())
            fail("must be an integer");
        m_int_val = m_root[m_name].asInt();
        m_type    = Type::Int;
        return *this;
    }

    BodyFieldValidator& require_uint64()
    {
        if (skip())
            return *this;

        if (!m_root.isMember(m_name))
            fail("is required");
        if (!m_root[m_name].isUInt64())
            fail("must be an integer");
        m_uint64_val = m_root[m_name].asUInt64();
        m_type       = Type::Uint64;
        return *this;
    }

    BodyFieldValidator& require_double()
    {
        if (skip())
            return *this;

        if (!m_root.isMember(m_name))
            fail("is required");
        // 数字字面量既可能是 int 也可能是 double，都接受
        if (!m_root[m_name].isNumeric())
            fail("must be a number");
        m_double_val = m_root[m_name].asDouble();
        m_type       = Type::Double;
        return *this;
    }

    BodyFieldValidator& require_bool()
    {
        if (skip())
            return *this;

        if (!m_root.isMember(m_name))
            fail("is required");
        if (!m_root[m_name].isBool())
            fail("must be a boolean");
        m_bool_val = m_root[m_name].asBool();
        m_type     = Type::Bool;
        return *this;
    }

    BodyFieldValidator& require_array()
    {
        if (skip())
            return *this;

        if (!m_root.isMember(m_name))
            fail("is required");
        if (!m_root[m_name].isArray())
            fail("must be an array");
        m_type = Type::Array;
        return *this;
    }

    // ---------------- 通用约束 ----------------

    // 非空：string 去空白后非空；array 元素数 > 0
    BodyFieldValidator& not_empty()
    {
        if (skip())
            return *this;

        switch (m_type)
        {
            case Type::String:
                if (utils::trim(m_val).empty())
                    fail("must not be empty");
                break;
            case Type::Array:
                if (m_root[m_name].empty())
                    fail("must not be empty");
                break;
            default:
                fail("notEmpty() only supports string or array");
        }
        return *this;
    }

    // 长度/数量上限：string 取字符数；array 取元素个数
    BodyFieldValidator& max_length(size_t n)
    {
        if (skip())
            return *this;

        switch (m_type)
        {
            case Type::String:
                if (m_val.size() > n)
                    fail("is too long (max " + std::to_string(n) + ")");
                break;
            case Type::Array:
                if (m_root[m_name].size() > n)
                    fail("has too many items (max " + std::to_string(n) + ")");
                break;
            default:
                fail("maxLength() only supports string or array");
        }
        return *this;
    }

    BodyFieldValidator& equal_length(size_t n)
    {
        if (skip())
            return *this;

        switch (m_type)
        {
            case Type::String:
                if (m_val.size() != n)
                    fail("is too long (max " + std::to_string(n) + ")");
                break;
            case Type::Array:
                if (m_root[m_name].size() != n)
                    fail("has too many items (max " + std::to_string(n) + ")");
                break;
            default:
                fail("equal_length() only supports string or array");
        }
        return *this;
    }

    // 数值范围（int / double）
    BodyFieldValidator& range(double lo, double hi)
    {
        if (skip())
            return *this;

        double v;
        if (m_type == Type::Int)
            v = static_cast<double>(m_int_val);
        else if (m_type == Type::Uint64)
            v = static_cast<double>(m_uint64_val);
        else if (m_type == Type::Double)
            v = m_double_val;
        else
            fail("range() only supports int or double");
        if (v < lo || v > hi)
            fail("must be in range [" + std::to_string(lo) + ", " + std::to_string(hi) + "]");
        return *this;
    }

    // 任意谓词（字符串场景沿用原有签名，传 std::string）
    template <typename Pred>
    BodyFieldValidator& must(Pred&& pred, std::string reason)
    {
        if (skip())
            return *this;

        if (!pred(m_val))
            fail(reason);

        return *this;
    }

    // 数组逐元素校验：复用 BodyFieldValidator，错误信息带下标
    template <typename Fn>
    BodyFieldValidator& each(Fn&& fn)
    {
        if (skip())
            return *this;

        if (m_type != Type::Array)
            fail("each() only supports array");
        const auto& arr = m_root[m_name];
        for (Json::ArrayIndex i = 0; i < arr.size(); ++i)
        {
            // 让回调拿到 "name[i]" 的 BodyFieldValidator，元素本身作为 root
            // 这里用一个小技巧：把元素包进一个临时对象，name 用 "name[i]"
            Json::Value wrapper;
            wrapper[m_name + "[" + std::to_string(i) + "]"] = arr[i];
            BodyFieldValidator elem(wrapper, m_name + "[" + std::to_string(i) + "]");
            fn(elem);
        }
        return *this;
    }

    // ---------------- 取值 ----------------

    std::string trimmed() const
    {
        return utils::trim(m_val);
    }

    const std::string& str() const
    {
        return m_val;
    }

    int asInt() const
    {
        return m_int_val;
    }

    uint64_t asUint64() const
    {
        return m_uint64_val;
    }

    double asDouble() const
    {
        return m_double_val;
    }

    bool asBool() const
    {
        return m_bool_val;
    }

    const Json::Value& arrayValue() const
    {
        return m_root[m_name];
    }

private:
    enum class Type
    {
        Unknown,
        String,
        Int,
        Uint64,
        Double,
        Bool,
        Array
    };

    [[noreturn]] void fail(const std::string& reason) const
    {
        throw common::AppException(common::ErrorCode::InvalidParam, m_name + " " + reason);
    }

    bool skip() const
    {
        // 可选且字段不存在 -> 跳过；必填且不存在 -> 报错
        if (m_optional && !m_present)
            return true;
        return false;
    }

private:
    const Json::Value& m_root;
    std::string        m_name;
    Type               m_type     = Type::Unknown;
    bool               m_optional = false;
    bool               m_present  = false;
    std::string        m_val;
    int                m_int_val    = 0;
    uint64_t           m_uint64_val = 0;
    double             m_double_val = 0.0;
    bool               m_bool_val   = false;
};

/// 从查询参数获取并校验
class QueryFieldValidator
{
public:
    QueryFieldValidator(const drogon::HttpRequestPtr& req, const std::string& name)
        : m_req(req), m_name(name)
    {
        m_present = m_req->getParameters().count(m_name) == 0 ? false : true;
    }

    QueryFieldValidator& optional()
    {
        m_optional = true;
        return *this;
    }

    QueryFieldValidator& require()
    {
        if (skip())
            return *this;

        if (!m_present)
            fail("is required");
        m_val = m_req->getParameter(m_name);
        return *this;
    }

    QueryFieldValidator& not_empty()
    {
        if (skip())
            return *this;

        if (utils::trim(m_val).empty())
            fail("must not be empty");

        return *this;
    }

    std::string str() const
    {
        return m_val;
    }

    std::string trimmed() const
    {
        return utils::trim(m_val);
    }

    bool asBool() const
    {
        const std::string s = utils::trim(m_val);
        if (s.empty())
            return false;
        if (s == "true" || s == "True" || s == "TRUE" || s == "1")
            return true;
        else if (s == "false" || s == "False" || s == "FALSE" || s == "0")
            return false;
        fail("must be a boolean");
    }

    int asInt() const
    {
        const std::string s = utils::trim(m_val);
        if (s.empty())
            return 0;
        int        v = 0;
        const auto r = std::from_chars(s.data(), s.data() + s.size(), v);
        if (r.ec != std::errc{} || r.ptr != s.data() + s.size())
            fail("must be an integer");
        return v;
    }

    uint64_t asUint64() const
    {
        const std::string s = utils::trim(m_val);
        if (s.empty())
            return 0;
        uint64_t   v = 0;
        const auto r = std::from_chars(s.data(), s.data() + s.size(), v);
        if (r.ec != std::errc{} || r.ptr != s.data() + s.size())
            fail("must be an integer");
        return v;
    }

    double asDouble() const
    {
        const std::string s = utils::trim(m_val);
        if (s.empty())
            return 0.0;
        double v   = 0.0;
        size_t pos = 0;
        try
        {
            v = std::stod(s, &pos);
        }
        catch (const std::exception&)
        {
            fail("must be a number");
        }
        if (pos != s.size())
            fail("must be a number");
        return v;
    }

private:
    [[noreturn]] void fail(const std::string& reason) const
    {
        throw common::AppException(common::ErrorCode::InvalidParam, m_name + " " + reason);
    }

    bool skip() const
    {
        // 可选且字段不存在 -> 跳过；必填且不存在 -> 报错
        if (m_optional && !m_present)
            return true;
        return false;
    }

private:
    const drogon::HttpRequestPtr& m_req;
    const std::string             m_name;
    std::string                   m_val;
    bool                          m_optional = false;
    bool                          m_present  = false;
};

/// 从路径参数获取并校验

/// handler 形参直接接收，类型可以是 std::string / int / int64_t / double 等，Drogon
/// 自动转换，转换失败返回 400。
}  // namespace

namespace common
{

// 便捷入口：一次性声明某个字段的完整规则
class BodyParams
{
public:
    explicit BodyParams(const Json::Value& body) : m_body(body)
    {
    }

    BodyFieldValidator field(const std::string& name)
    {
        return BodyFieldValidator(m_body, name);
    }

    const Json::Value& raw() const
    {
        return m_body;
    }

private:
    const Json::Value& m_body;
};

class QueryParams
{
public:
    explicit QueryParams(const drogon::HttpRequestPtr& req) : m_req(req)
    {
    }

    QueryFieldValidator filed(const std::string& name)
    {
        return QueryFieldValidator(m_req, name);
    }

    const drogon::HttpRequestPtr& raw() const
    {
        return m_req;
    }

private:
    const drogon::HttpRequestPtr& m_req;
};

}  // namespace common