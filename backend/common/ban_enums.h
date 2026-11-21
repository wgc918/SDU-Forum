// ban_enums.h
#pragma once

#include <cstdint>
#include <optional>
#include <string>

namespace ban
{

// ============================================================
// 枚举定义（只增不改，废弃码保留值并在映射表标记 deprecated）
// ============================================================

/// 封禁原因分类码
/// 分段：1xxx 内容违规 / 2xxx 行为违规 / 3xxx 账号安全 / 4xxx 系统自动 / 5xxx 其他
enum class ReasonCode : uint16_t
{
    Unspecified = 0,  // 自定义原因，展示 reason 原文

    // --- 1xxx 内容违规 ---
    SpamAd         = 1001,  // 发布广告或垃圾信息
    IllegalContent = 1002,  // 发布违法违规内容
    Porn           = 1003,  // 发布色情低俗内容
    Violence       = 1004,  // 发布暴力或血腥内容
    Political      = 1005,  // 发布不当政治言论
    Copyright      = 1006,  // 侵犯他人知识产权
    Privacy        = 1007,  // 泄露他人隐私信息
    Misinformation = 1008,  // 传播虚假信息/谣言

    // --- 2xxx 行为违规 ---
    PersonalAttack  = 2001,  // 人身攻击或辱骂
    Harassment      = 2002,  // 骚扰他人
    HateSpeech      = 2003,  // 仇恨言论
    Impersonation   = 2004,  // 冒充他人或官方
    RepeatViolation = 2005,  // 多次违规屡教不改
    MaliciousReport = 2006,  // 恶意举报/滥用举报
    Trolling        = 2007,  // 恶意引战/灌水刷屏

    // --- 3xxx 账号安全 ---
    AccountTheft  = 3001,  // 账号被盗风险
    SharedAccount = 3002,  // 账号共享/买卖
    BotActivity   = 3003,  // 疑似机器人/脚本行为
    SecurityRisk  = 3004,  // 账号存在安全风险

    // --- 4xxx 系统自动 ---
    AutoRuleHit     = 4001,  // 触发系统自动风控规则
    ReportThreshold = 4002,  // 被多人举报且核实

    // --- 5xxx 其他 ---
    UserRequest  = 5001,  // 用户主动申请
    Inactive     = 5002,  // 长期未登录清理
    Other        = 5003,  // 其他违规行为
    ManualCustom = 5999,  // 管理员自定义（配合 reason 原文）
};

/// 封禁维度（动作类型）
enum class BanType : uint8_t
{
    All            = 0,  // 全站封禁
    Comment        = 1,  // 禁止评论
    Post           = 2,  // 禁止发帖
    PrivateMessage = 3,  // 禁止私信
    Login          = 4,  // 禁止登录
    ProfileEdit    = 5,  // 禁止修改资料
    Upload         = 6,  // 禁止上传附件/图片
    ReadOnly       = 7,  // 只读模式
};

/// 封禁来源
enum class BanSource : uint8_t
{
    Manual = 0,  // 管理员手动
    Auto   = 1,  // 系统自动
    Report = 2,  // 举报触发
};

/// 封禁状态（对应表 status）
enum class BanStatus : uint8_t
{
    Active  = 1,  // 生效中
    Revoked = 0,  // 已解除
    Expired = 2,  // 已过期
};

// ============================================================
// 映射条目 & 元数据
// ============================================================

struct ReasonCodeMeta
{
    ReasonCode  code;
    std::string display_key;         // i18n key，如 "ban.reason.spam_ad"
    std::string default_text;        // 中文兜底文案
    bool        deprecated = false;  // 废弃码
    bool        auto_only  = false;  // 仅允许系统自动写入
};

struct BanTypeMeta
{
    BanType     type;
    std::string display_key;
    std::string default_text;
    bool        deprecated = false;
};

// ============================================================
// 映射查询
// ============================================================

/// 查 ReasonCode 元数据；非法码返回 std::nullopt
inline std::optional<ReasonCodeMeta> FindReasonMeta(ReasonCode code)
{
    // 使用静态表，O(n) 或转 unordered_map 均可
    static const ReasonCodeMeta kTable[] = {
        {ReasonCode::Unspecified, "ban.reason.unspecified", "（自定义原因）"},
        {ReasonCode::SpamAd, "ban.reason.spam_ad", "发布广告或垃圾信息"},
        {ReasonCode::IllegalContent, "ban.reason.illegal_content", "发布违法违规内容"},
        {ReasonCode::Porn, "ban.reason.porn", "发布色情低俗内容"},
        {ReasonCode::Violence, "ban.reason.violence", "发布暴力或血腥内容"},
        {ReasonCode::Political, "ban.reason.political", "发布不当政治言论"},
        {ReasonCode::Copyright, "ban.reason.copyright", "侵犯他人知识产权"},
        {ReasonCode::Privacy, "ban.reason.privacy", "泄露他人隐私信息"},
        {ReasonCode::Misinformation, "ban.reason.misinformation", "传播虚假信息/谣言"},
        {ReasonCode::PersonalAttack, "ban.reason.personal_attack", "人身攻击或辱骂"},
        {ReasonCode::Harassment, "ban.reason.harassment", "骚扰他人"},
        {ReasonCode::HateSpeech, "ban.reason.hate_speech", "仇恨言论"},
        {ReasonCode::Impersonation, "ban.reason.impersonation", "冒充他人或官方"},
        {ReasonCode::RepeatViolation, "ban.reason.repeat_violation", "多次违规屡教不改"},
        {ReasonCode::MaliciousReport, "ban.reason.malicious_report", "恶意举报/滥用举报"},
        {ReasonCode::Trolling, "ban.reason.trolling", "恶意引战/灌水刷屏"},
        {ReasonCode::AccountTheft, "ban.reason.account_theft", "账号被盗风险"},
        {ReasonCode::SharedAccount, "ban.reason.shared_account", "账号共享/买卖"},
        {ReasonCode::BotActivity, "ban.reason.bot_activity", "疑似机器人/脚本行为"},
        {ReasonCode::SecurityRisk, "ban.reason.security_risk", "账号存在安全风险"},
        {ReasonCode::AutoRuleHit, "ban.reason.auto_rule_hit", "触发系统自动风控规则", false, true},
        {ReasonCode::ReportThreshold, "ban.reason.report_threshold", "被多人举报且核实", false,
         true},
        {ReasonCode::UserRequest, "ban.reason.user_request", "用户主动申请"},
        {ReasonCode::Inactive, "ban.reason.inactive", "长期未登录清理"},
        {ReasonCode::Other, "ban.reason.other", "其他违规行为"},
        {ReasonCode::ManualCustom, "ban.reason.manual_custom", "（管理员自定义）"},
    };
    for (const auto& m : kTable)
    {
        if (m.code == code)
            return m;
    }
    return std::nullopt;
}

inline std::optional<BanTypeMeta> FindBanTypeMeta(BanType type)
{
    static const BanTypeMeta kTable[] = {
        {BanType::All,            "ban.type.all",             "全站封禁"         },
        {BanType::Comment,        "ban.type.comment",         "禁止评论"         },
        {BanType::Post,           "ban.type.post",            "禁止发帖"         },
        {BanType::PrivateMessage, "ban.type.private_message", "禁止私信"         },
        {BanType::Login,          "ban.type.login",           "禁止登录"         },
        {BanType::ProfileEdit,    "ban.type.profile_edit",    "禁止修改资料"     },
        {BanType::Upload,         "ban.type.upload",          "禁止上传附件/图片"},
        {BanType::ReadOnly,       "ban.type.read_only",       "只读模式"         },
    };
    for (const auto& m : kTable)
    {
        if (m.type == type)
            return m;
    }
    return std::nullopt;
}

/// 取对外展示文案（中文兜底）；非法码返回空
inline std::string ReasonDisplayText(ReasonCode code)
{
    auto m = FindReasonMeta(code);
    return m ? m->default_text : std::string{};
}

inline std::string BanTypeDisplayText(BanType type)
{
    auto m = FindBanTypeMeta(type);
    return m ? m->default_text : std::string{};
}

// ============================================================
// 校验
// ============================================================

/// reason_code 是否为合法枚举值
inline bool IsReasonCodeValid(uint16_t raw)
{
    return FindReasonMeta(static_cast<ReasonCode>(raw)).has_value();
}

/// ban_type 是否为合法枚举值
inline bool IsBanTypeValid(uint8_t raw)
{
    return FindBanTypeMeta(static_cast<BanType>(raw)).has_value();
}

/// 是否为系统自动类原因（4xxx）
inline bool IsAutoReason(ReasonCode code)
{
    auto v = static_cast<uint16_t>(code);
    return v >= 4000 && v < 5000;
}

/// source 与 reason_code 的一致性校验
/// 规则：auto_only 的码必须 source=Auto/Report；4xxx 必须 source=Auto/Report
inline bool CheckSourceConsistency(ReasonCode code, BanSource source)
{
    auto m = FindReasonMeta(code);
    if (!m)
        return false;
    if (m->auto_only)
    {
        return source == BanSource::Auto || source == BanSource::Report;
    }
    if (IsAutoReason(code))
    {
        return source == BanSource::Auto || source == BanSource::Report;
    }
    return true;
}

/// reason 原文与 reason_code 的关系校验
/// 约定：code==Unspecified 或 ManualCustom 时，必须提供非空 reason 原文；
///       其余标准码，reason 可为空（前端优先展示标准文案）
inline bool CheckReasonTextConsistency(ReasonCode code, std::string reason)
{
    if (code == ReasonCode::Unspecified || code == ReasonCode::ManualCustom)
    {
        return !reason.empty();
    }
    return true;
}

/// 写库前的整体校验
struct BanRecordInput
{
    uint16_t    reason_code = 0;
    std::string reason;
    uint8_t     ban_type = 0;
    BanSource   source   = BanSource::Manual;
    // expire_at 等时间字段由业务层另外校验
};

inline bool ValidateBanInput(const BanRecordInput& in, std::string* err = nullptr)
{
    auto fail = [&](const char* msg)
    {
        if (err)
            *err = msg;
        return false;
    };

    if (!IsReasonCodeValid(in.reason_code))
        return fail("invalid reason_code");

    if (!IsBanTypeValid(in.ban_type))
        return fail("invalid ban_type");

    auto code = static_cast<ReasonCode>(in.reason_code);
    if (!CheckSourceConsistency(code, in.source))
        return fail("source inconsistent with reason_code");

    if (!CheckReasonTextConsistency(code, in.reason))
        return fail("reason text required for custom reason code");

    if (in.reason.size() > 255)
        return fail("reason too long (>255)");

    return true;
}

}  // namespace ban