/* ==========================================================================
   SDU Forum Admin · 模拟数据
   所有页面数据均来自本文件暴露的全局常量，禁止写死在 HTML 中
   ========================================================================== */
(function () {
  'use strict';

  const NOW = Date.now();
  const DAY = 24 * 60 * 60 * 1000;

  /* ---------- 可复现随机 ---------- */
  let seed = 20261123;
  function rnd() {
    seed = (seed * 9301 + 49297) % 233280;
    return seed / 233280;
  }
  function ri(min, max) {
    return Math.floor(rnd() * (max - min + 1)) + min;
  }
  function pick(list) {
    return list[Math.floor(rnd() * list.length)];
  }
  function chance(percent) {
    return rnd() < percent;
  }
  function ago(days, hours) {
    return NOW - Math.round((days || 0) * DAY + (hours || 0) * 3600000);
  }

  /* ---------- 基础字典 ---------- */
  const COLLEGES = [
    { name: '计算机科学与技术学院', majors: ['计算机科学与技术', '人工智能', '数据科学与大数据技术'] },
    { name: '软件学院', majors: ['软件工程', '数字媒体技术', '网络空间安全'] },
    { name: '网络空间安全学院', majors: ['信息安全', '密码科学与技术'] },
    { name: '数学学院', majors: ['数学与应用数学', '统计学', '信息与计算科学'] },
    { name: '物理学院', majors: ['物理学', '应用物理学'] },
    { name: '化学与化工学院', majors: ['化学', '化学工程与工艺', '应用化学'] },
    { name: '生命科学学院', majors: ['生物科学', '生物技术', '生态学'] },
    { name: '机械工程学院', majors: ['机械设计制造及其自动化', '工业设计', '车辆工程'] },
    { name: '控制科学与工程学院', majors: ['自动化', '测控技术与仪器', '生物医学工程'] },
    { name: '经济学院', majors: ['经济学', '金融学', '国际经济与贸易'] },
    { name: '管理学院', majors: ['工商管理', '会计学', '市场营销', '信息管理与信息系统'] },
    { name: '文学院', majors: ['汉语言文学', '新闻学', '广告学'] },
    { name: '外国语学院', majors: ['英语', '日语', '翻译'] },
    { name: '法学院', majors: ['法学', '知识产权'] },
    { name: '医学院', majors: ['临床医学', '口腔医学', '预防医学'] },
    { name: '哲学与社会发展学院', majors: ['哲学', '社会学', '社会工作'] }
  ];

  const SURNAMES = ['王', '李', '张', '刘', '陈', '杨', '赵', '黄', '周', '吴', '徐', '孙', '马', '朱', '胡', '郭', '林', '何', '高', '罗', '郑', '梁', '谢', '宋', '唐', '许', '韩', '冯', '邓', '曹'];
  const GIVEN_NAMES = ['子涵', '雨桐', '浩然', '思远', '嘉怡', '明轩', '欣怡', '宇航', '若曦', '俊杰', '梓萱', '博文', '诗涵', '天佑', '雅琪', '晨曦', '一诺', '梦瑶', '泽宇', '静怡', '文博', '嘉琪', '子墨', '昕怡', '皓宇', '语嫣', '书豪', '心悦', '奕辰', '安然', '知远', '清越'];
  const NICK_PREFIX = ['图书馆的', '深夜的', '考研的', '山大的', '操场边的', '食堂里的', '宿舍楼的', '教室里的', '春天的', '秋天的'];
  const NICK_SUFFIX = ['小猫', '夜行者', '奋斗者', '观察员', '旅人', '柠檬', '小鹿', '学霸', '路人', '阿呆', '月亮', '橘子'];
  const BIOS = ['在代码与生活之间反复横跳。', '认真生活，慢慢变好。', '喜欢在图书馆五楼靠窗的位置。', '一个还在找方向的人。', '多读书，少熬夜。', '想去看更大的世界。', '偶尔发帖，长期潜水。', '愿每一次努力都有回响。', ''];

  const TAG_SEED = [
    ['选课攻略', '校园生活'], ['食堂测评', '校园生活'], ['宿舍生活', '校园生活'], ['校园跑', '校园生活'],
    ['图书馆', '校园生活'], ['校车出行', '校园生活'], ['二手交易', '校园生活'], ['失物招领', '校园生活'],
    ['考研交流', '学习科研'], ['保研经验', '学习科研'], ['四六级', '学习科研'], ['期末复习', '学习科研'],
    ['实验室', '学习科研'], ['论文写作', '学习科研'], ['交换生', '学习科研'], ['奖学金', '学习科研'],
    ['实习经验', '求职就业'], ['校园招聘', '求职就业'], ['简历修改', '求职就业'], ['考公考编', '求职就业'],
    ['社团招新', '兴趣社团'], ['音乐现场', '兴趣社团'], ['篮球联赛', '兴趣社团'], ['摄影', '兴趣社团'],
    ['读书会', '兴趣社团'], ['桌游', '兴趣社团'], ['志愿服务', '兴趣社团'], ['校园活动', '兴趣社团'],
    ['情感树洞', '情感树洞'], ['匿名倾诉', '情感树洞'], ['新生指南', '校园生活'], ['山大气象', '校园生活'],
    ['校园网', '校园生活'], ['通知公告', '站务公告'], ['意见反馈', '站务公告'], ['站务讨论', '站务公告']
  ];

  const TAG_COLORS = ['#1E5BB8', '#0F766E', '#B45309', '#7C3AED', '#BE123C', '#0369A1', '#4D7C0F', '#9A3412'];

  /* ========================================================================
     角色与管理员
     ======================================================================== */
  const ROLES = [
    { id: 'role1', name: '超级管理员', key: 'super_admin', description: '拥有系统全部权限，可管理管理员与角色配置', builtin: true, permissions: ['用户管理', '内容管理', '举报处理', '公告管理', '数据统计', '系统设置', '日志审计', '角色配置'], memberCount: 1, createdAt: ago(720) },
    { id: 'role2', name: '内容管理员', key: 'content_admin', description: '负责帖子、评论、标签与敏感词的内容治理', builtin: false, permissions: ['内容管理', '标签管理', '举报处理', '数据统计'], memberCount: 2, createdAt: ago(680) },
    { id: 'role3', name: '用户管理员', key: 'user_admin', description: '负责用户资料、封禁与实名认证审核', builtin: false, permissions: ['用户管理', '封禁管理', '实名认证', '日志审计'], memberCount: 2, createdAt: ago(650) },
    { id: 'role4', name: '内容审核员', key: 'reviewer', description: '处理待审核帖子、评论与举报工单', builtin: false, permissions: ['内容审核', '举报处理'], memberCount: 3, createdAt: ago(600) },
    { id: 'role5', name: '运营编辑', key: 'operator', description: '维护公告、运营位与社区活动', builtin: false, permissions: ['公告管理', '数据统计', '用户管理'], memberCount: 1, createdAt: ago(540) },
    { id: 'role6', name: '数据分析师', key: 'analyst', description: '只读访问数据统计与导出报表', builtin: false, permissions: ['数据统计'], memberCount: 1, createdAt: ago(480) },
    { id: 'role7', name: '客服专员', key: 'support', description: '处理用户反馈与账号问题咨询', builtin: false, permissions: ['用户反馈', '用户管理'], memberCount: 1, createdAt: ago(420) },
    { id: 'role8', name: '只读观察员', key: 'viewer', description: '仅可查看后台数据，不能执行写操作', builtin: false, permissions: ['数据统计', '日志审计'], memberCount: 1, createdAt: ago(360) }
  ];

  const ADMIN_NAMES = ['张明远', '李思静', '王浩然', '陈雨欣', '刘博文', '赵雅琪', '孙宇航', '周静怡', '吴嘉豪', '郑若曦', '冯子墨', '韩心悦'];

  const ADMINS = ADMIN_NAMES.map(function (name, index) {
    const role = ROLES[index === 0 ? 0 : (index % (ROLES.length - 1)) + 1];
    return {
      id: 'a' + (index + 1),
      name: name,
      account: index === 0 ? 'admin' : role.key + '.' + (index + 1),
      roleId: role.id,
      roleName: role.name,
      email: (index === 0 ? 'admin' : role.key + (index + 1)) + '@sdu.edu.cn',
      status: index === 10 ? 'disabled' : 'active',
      lastLoginAt: index === 0 ? ago(0, 0.2) : ago(ri(0, 12), ri(0, 23)),
      createdAt: ago(ri(120, 720))
    };
  });

  /* ========================================================================
     标签
     ======================================================================== */
  const TAGS = TAG_SEED.map(function (item, index) {
    return {
      id: 't' + (index + 1),
      name: item[0],
      category: item[1],
      color: TAG_COLORS[index % TAG_COLORS.length],
      description: '关于「' + item[0] + '」的讨论与经验分享',
      postCount: 0,
      followCount: ri(80, 3200),
      status: index === 34 ? 'hidden' : 'active',
      moderator: ADMINS[index % ADMINS.length].name,
      createdAt: ago(ri(200, 900))
    };
  });

  /* ========================================================================
     用户（1,286 人）
     ======================================================================== */
  const USERS = [];
  const usedStudentIds = {};
  const personalPool = [];
  for (let i = 0; i < 600; i++) personalPool.push(pick(SURNAMES) + pick(GIVEN_NAMES));

  for (let i = 1; i <= 1286; i++) {
    const college = COLLEGES[i % COLLEGES.length];
    const year = ri(2019, 2025);
    const major = pick(college.majors);
    const realName = personalPool[i % personalPool.length];
    let nickname;
    if (chance(0.55)) nickname = pick(NICK_PREFIX) + pick(NICK_SUFFIX);
    else nickname = pick(SURNAMES) + pick(GIVEN_NAMES);

    let studentId;
    do {
      studentId = String(year) + String(10 + (i % COLLEGES.length)).padStart(2, '0') + String(ri(100000, 399999));
    } while (usedStudentIds[studentId]);
    usedStudentIds[studentId] = true;

    let role = 'user';
    if (i === 1) role = 'admin';
    else if (chance(0.025)) role = 'moderator';

    let status = 'active';
    if (role === 'user') {
      const roll = rnd();
      if (roll < 0.032) status = 'banned';
      else if (roll < 0.075) status = 'muted';
      else if (roll < 0.115) status = 'pending';
    }

    const registerAt = i <= 52
      ? ago(ri(0, 29), ri(0, 23))
      : ago(Math.pow(rnd(), 1.55) * 1080, ri(0, 23));

    const lastActiveAt = status === 'active'
      ? ago(rnd() * 9, ri(0, 23))
      : ago(rnd() * 45, ri(0, 23));

    USERS.push({
      id: 'u' + i,
      studentId: studentId,
      nickname: nickname,
      realName: realName,
      college: college.name,
      major: major,
      grade: year + '级',
      role: role,
      status: status,
      postCount: 0,
      commentCount: 0,
      reputation: ri(0, 2600),
      verified: chance(0.72),
      email: studentId + '@mail.sdu.edu.cn',
      bio: pick(BIOS),
      registerAt: registerAt,
      lastActiveAt: lastActiveAt,
      avatar: ''
    });
  }

  /* ========================================================================
     帖子
     ======================================================================== */
  const POST_TOPICS = ['选课', '食堂', '考研', '保研', '实习', '宿舍', '图书馆', '校车', '社团招新', '体测', '四六级', '期末复习', '实验室', '交换生', '奖学金', '校园网', '外卖', '夜跑', '校园活动', '志愿服务'];
  const POST_PATTERNS = [
    '关于{topic}，想听听大家的意见',
    '{topic}求助：有没有过来人分享一下经验？',
    '分享｜我整理的一份{topic}攻略',
    '{topic}这件事，学校终于有回应了',
    '今天在{topic}遇到一件很暖心的事',
    '吐槽一下{topic}的体验，实在忍不住了',
    '有没有同学一起约{topic}？',
    '{topic}避坑指南（长文）',
    '认真问一下，{topic}到底该怎么选？',
    '记录一下我的{topic}经历'
  ];

  const postPool = USERS.filter(function (user) {
    return user.status === 'active' || user.status === 'muted';
  });

  const POSTS = [];
  for (let i = 1; i <= 240; i++) {
    const author = pick(postPool);
    const tag = pick(TAGS);
    const topic = pick(POST_TOPICS);
    const title = pick(POST_PATTERNS).replace('{topic}', topic);
    const createdDays = i <= 3 ? 0 : Math.pow(rnd(), 1.4) * 120;
    const createdAt = ago(createdDays, ri(0, 23));
    let status = 'published';
    if (i % 17 === 0) status = 'pending';
    else if (i % 29 === 0) status = 'hidden';
    else if (i % 41 === 0) status = 'rejected';

    POSTS.push({
      id: 'p' + i,
      title: title,
      summary: title + '。' + pick(['欢迎大家理性讨论，友善交流。', '以下是我的一些个人看法，仅供参考。', '如果有不准确的地方，欢迎指正。', '希望能帮到有需要的同学。']),
      authorId: author.id,
      authorName: author.nickname,
      authorStudentId: author.studentId,
      tagId: tag.id,
      tagName: tag.name,
      board: tag.category,
      createdAt: createdAt,
      updatedAt: createdAt + ri(0, 5) * 3600000,
      viewCount: status === 'published' ? ri(60, 8600) : ri(5, 400),
      likeCount: ri(0, 520),
      commentCount: 0,
      status: status,
      pinned: chance(0.02),
      essence: chance(0.08),
      ip: '112.231.' + ri(1, 254) + '.' + ri(1, 254)
    });
    tag.postCount += 1;
    author.postCount += 1;
  }

  /* ========================================================================
     评论
     ======================================================================== */
  const COMMENT_TEXTS = [
    '同意楼主，我也有同感。',
    '这个信息很有用，感谢分享！',
    '我觉得还是要看具体情况，不能一概而论。',
    '接楼上的说法，补充一点自己的经历。',
    '求问具体是在哪个校区？',
    '已经收藏，准备周末去试试。',
    '哈哈哈太真实了，笑死。',
    '同学你好，可以私信交流一下吗？',
    '这条评论有点引战了，注意言辞。',
    '学校最近确实在改，期待后续。',
    '有没有官方渠道可以确认一下？',
    '去年也是这样，后来不了了之了。',
    '建议直接联系辅导员处理。',
    '感谢提醒，差点踩坑。',
    '从数据上看，这个说法不太成立。',
    '我在现场，情况和楼主描述的略有出入。',
    '顶一个，希望更多人看到。',
    '这个帖子应该加精。'
  ];

  const publishedPosts = POSTS.filter(function (post) { return post.status === 'published'; });
  const COMMENTS = [];
  for (let i = 1; i <= 480; i++) {
    const post = pick(publishedPosts);
    const author = pick(postPool);
    const createdAt = post.createdAt + Math.floor(rnd() * Math.max(NOW - post.createdAt, 1));
    let status = 'normal';
    if (i % 23 === 0) status = 'hidden';
    else if (i % 31 === 0) status = 'reviewing';

    COMMENTS.push({
      id: 'c' + i,
      postId: post.id,
      postTitle: post.title,
      authorId: author.id,
      authorName: author.nickname,
      content: pick(COMMENT_TEXTS),
      createdAt: Math.min(createdAt, NOW - 60000),
      likeCount: ri(0, 96),
      status: status,
      reportCount: chance(0.12) ? ri(1, 5) : 0,
      ip: '112.231.' + ri(1, 254) + '.' + ri(1, 254)
    });
    post.commentCount += 1;
    author.commentCount += 1;
  }

  /* ========================================================================
     封禁记录
     ======================================================================== */
  const BAN_TYPES = ['login', 'post', 'comment', 'all'];
  const BAN_REASONS = ['发布广告引流内容', '恶意辱骂他人', '多次发布不实信息', '刷屏灌水', '账号存在被盗风险', '侵犯他人隐私', '发布违规交易信息', '恶意举报骚扰用户'];
  const BAN_DETAILS = ['经举报核实，账号在多个帖子下重复发布引流二维码。', '评论区多次出现人身攻击，警告后仍未改正。', '系统检测到异常登录行为，为保护账号安全临时封禁。', '短时间内高频发布重复内容，影响社区秩序。', '用户申诉后复核，维持原处理结果。'];

  const BANS = [];
  const bannedUsers = USERS.filter(function (user) { return user.status === 'banned'; });
  const banTargets = bannedUsers.concat(USERS.filter(function (user) { return user.status !== 'banned'; }).slice(0, 400));
  for (let i = 1; i <= 36; i++) {
    const user = i <= bannedUsers.length ? bannedUsers[i - 1] : pick(banTargets);
    const startDays = ri(1, 150);
    const startAt = ago(startDays, ri(0, 23));
    const duration = pick([3, 7, 15, 30, 90, 0]);
    const endAt = duration === 0 ? null : startAt + duration * DAY;
    let status = 'active';
    if (chance(0.18)) status = 'revoked';
    else if (endAt && endAt < NOW) status = 'expired';

    BANS.push({
      id: 'b' + i,
      userId: user.id,
      userName: user.nickname,
      studentId: user.studentId,
      type: pick(BAN_TYPES),
      reason: pick(BAN_REASONS),
      detail: pick(BAN_DETAILS),
      operator: pick(ADMINS).name,
      startAt: startAt,
      endAt: endAt,
      duration: duration === 0 ? '永久' : duration + ' 天',
      status: status,
      evidence: '举报工单 #' + (1000 + i),
      createdAt: startAt
    });
  }

  /* ========================================================================
     举报
     ======================================================================== */
  const REPORT_REASONS = [
    ['广告引流', '内容包含二维码与外部引流链接'],
    ['辱骂攻击', '评论区存在人身攻击与侮辱性词汇'],
    ['不实信息', '发布未经证实的校园通知'],
    ['色情低俗', '配图或文字包含低俗内容'],
    ['侵犯隐私', '未经允许公开他人姓名与联系方式'],
    ['违规交易', '涉及代写、代考等违规服务'],
    ['恶意刷屏', '短时间重复发布相同内容']
  ];

  const REPORTS = [];
  for (let i = 1; i <= 36; i++) {
    const isPost = chance(0.6);
    const target = isPost ? pick(POSTS) : pick(COMMENTS);
    const reporter = pick(USERS);
    const reason = pick(REPORT_REASONS);
    let status = 'pending';
    if (i % 10 === 0) status = 'rejected';
    else if (i % 4 === 0) status = 'processing';
    else if (i % 3 === 0) status = 'resolved';

    REPORTS.push({
      id: 'r' + i,
      targetType: isPost ? 'post' : 'comment',
      targetId: target.id,
      targetTitle: isPost ? target.title : target.content,
      targetAuthor: isPost ? target.authorName : target.authorName,
      reporterId: reporter.id,
      reporterName: reporter.nickname,
      reasonType: reason[0],
      reason: reason[1],
      status: status,
      priority: i % 7 === 0 ? 'high' : (i % 3 === 0 ? 'low' : 'mid'),
      handler: status === 'pending' ? '' : pick(ADMINS).name,
      result: status === 'resolved' ? '内容已下架并对作者进行警告' : (status === 'rejected' ? '经复核未发现违规' : ''),
      createdAt: ago(ri(0, 20), ri(0, 23))
    });
  }

  /* ========================================================================
     用户反馈
     ======================================================================== */
  const FEEDBACK_TYPES = ['bug', 'feature', 'experience', 'other'];
  const FEEDBACK_TITLES = ['移动端页面偶尔无法加载图片', '希望增加帖子收藏分组功能', '搜索结果的排序方式建议优化', '夜间模式下部分文字对比度偏低', '建议开放更多社团认证入口', '举报处理进度希望可以查询', '登录时验证码偶尔不显示', '希望支持导出个人发帖记录', '帖子编辑器建议增加草稿自动保存', '希望增加按学院筛选的板块', '消息通知希望能按类型关闭', '建议增加校友身份认证', '反馈入口在手机上不太容易找到', '希望帖子支持修改标签', '希望增加内容订阅功能', '个人主页加载速度有点慢', '建议优化图片上传的压缩策略', '希望举报时可以选择具体条款', '搜索历史记录希望能一键清空', '建议增加校园活动日历', '希望支持多设备登录管理', '帖子草稿箱入口不够明显', '希望增加匿名提问的说明', '评论区排序建议增加最新优先', '希望后台能看到反馈处理进度', '建议增加志愿时长记录功能', '希望优化夜间模式下的表格样式', '建议支持批量删除自己的评论', '希望增加失物招领的置顶规则', '帖子发布失败时提示不够明确'];
  const FEEDBACK_CONTENTS = [
    '在校园网环境下复现概率较高，切换网络后有所缓解，希望排查一下。',
    '这个需求在同学中讨论得比较多，如果能实现会方便很多。',
    '整体体验不错，但细节上还有优化空间，附上了截图。',
    '已经尝试退出重进，问题仍然存在，麻烦技术同学看一下。',
    '感谢运营同学的及时回复，期待后续版本更新。'
  ];

  const FEEDBACKS = FEEDBACK_TITLES.map(function (title, index) {
    const user = pick(USERS);
    const status = index % 5 === 0 ? 'pending' : (index % 5 === 1 ? 'processing' : 'resolved');
    return {
      id: 'f' + (index + 1),
      userId: user.id,
      userName: user.nickname,
      type: FEEDBACK_TYPES[index % FEEDBACK_TYPES.length],
      title: title,
      content: pick(FEEDBACK_CONTENTS),
      contact: chance(0.6) ? user.email : '',
      status: status,
      reply: status === 'resolved' ? '感谢反馈，相关优化已排入下个版本。' : '',
      handler: status === 'pending' ? '' : pick(ADMINS).name,
      createdAt: ago(ri(0, 45), ri(0, 23))
    };
  });

  /* ========================================================================
     公告
     ======================================================================== */
  const ANNOUNCEMENT_TITLES = [
    '关于校园论坛实名认证升级的公告', 'SDU Forum 社区管理规范（2026 修订版）', '国庆假期论坛值班安排', '关于严厉打击代写代考信息的公告',
    '论坛新版编辑器上线说明', '关于开展社区内容专项整治的通知', '校园网维护期间论坛访问说明', '关于新增「考研交流」板块的公告',
    '2026 年秋季学期版主招募公告', '关于优化举报处理流程的公告', '论坛用户等级体系调整说明', '关于清理长期未登录账号的公告',
    '校园活动发布规范更新', '关于图片上传大小限制调整的说明', '论坛隐私政策更新公告', '关于禁止发布违规交易信息的公告',
    '寒假期间论坛运营安排', '关于启用新版用户主页的公告', '社区优质内容激励计划上线', '关于规范社团账号认证的公告',
    '论坛搜索功能升级完成', '关于处理恶意举报行为的公告', '新生入学季论坛使用指南', '关于增加夜间模式的公告',
    '论坛服务器扩容完成通知', '关于调整帖子审核时效的公告', '「山大故事」征集活动开始', '关于完善用户申诉渠道的公告',
    '论坛年度数据报告发布', '关于春节期间值班与审核安排的公告'
  ];

  const ANNOUNCEMENT_POSITIONS = ['banner', 'modal', 'sidebar'];
  const ANNOUNCEMENT_GROUPS = ['2026 级新生', '毕业生', '已认证用户', '版主团队', '高活跃用户', '沉睡用户'];

  const ANNOUNCEMENTS = ANNOUNCEMENT_TITLES.map(function (title, index) {
    const createdAt = ago(ri(0, 120), ri(0, 23));
    const offset = ri(-38, 55);
    const startAt = NOW + offset * DAY + ri(8, 10) * 3600000;
    const endAt = startAt + ri(3, 40) * DAY;
    const positions = [ANNOUNCEMENT_POSITIONS[index % ANNOUNCEMENT_POSITIONS.length]];
    if (index % 4 === 0) {
      const extra = ANNOUNCEMENT_POSITIONS[(index + 1) % ANNOUNCEMENT_POSITIONS.length];
      if (positions.indexOf(extra) === -1) positions.push(extra);
    }
    const groups = index % 3 === 0 ? [] : [
      ANNOUNCEMENT_GROUPS[index % ANNOUNCEMENT_GROUPS.length],
      ANNOUNCEMENT_GROUPS[(index + 2) % ANNOUNCEMENT_GROUPS.length]
    ].filter(function (name, position, list) { return list.indexOf(name) === position; });

    return {
      id: 'n' + (index + 1),
      title: title,
      summary: '为维护良好的社区秩序，现将相关安排与规范说明如下，请各位同学知悉并遵守。',
      content: title + '\n\n为维护良好的社区秩序，保障广大同学的正常使用体验，现就相关事项说明如下：\n1. 请遵守社区管理规范，理性发言、友善交流。\n2. 如发现违规内容，可通过举报入口提交，我们会尽快处理。\n3. 如有疑问，可在「意见反馈」板块留言。',
      level: index % 6 === 0 ? 'important' : (index % 4 === 0 ? 'normal' : 'info'),
      status: startAt > NOW ? 'scheduled' : (endAt < NOW ? 'ended' : 'active'),
      author: pick(ADMINS).name,
      creator: pick(ADMINS).name,
      views: ri(120, 12800),
      createdAt: createdAt,
      publishedAt: startAt <= NOW ? startAt : null,
      positions: positions,
      scope: groups.length ? { type: 'groups', groups: groups } : { type: 'all', groups: [] },
      startAt: startAt,
      endAt: endAt,
      enabled: index % 11 !== 0
    };
  });

  /* ========================================================================
     操作日志
     ======================================================================== */
  const LOG_ACTIONS = [
    ['登录后台', '认证中心'], ['退出后台', '认证中心'], ['封禁用户', '用户管理'], ['解除封禁', '用户管理'],
    ['删除帖子', '内容管理'], ['置顶帖子', '内容管理'], ['审核通过', '内容管理'], ['驳回帖子', '内容管理'],
    ['隐藏评论', '内容管理'], ['处理举报', '社区治理'], ['回复反馈', '社区治理'], ['发布公告', '运营中心'],
    ['下线公告', '运营中心'], ['修改角色权限', '系统设置'], ['导出用户数据', '数据统计'], ['新增敏感词', '内容管理'],
    ['重置用户密码', '用户管理'], ['调整标签', '内容管理']
  ];

  const LOGS = [];
  for (let i = 1; i <= 80; i++) {
    const admin = pick(ADMINS);
    const action = pick(LOG_ACTIONS);
    LOGS.push({
      id: 'l' + i,
      adminId: admin.id,
      adminName: admin.name,
      action: action[0],
      module: action[1],
      target: pick(['用户 u' + ri(1, 1286), '帖子 p' + ri(1, 240), '评论 c' + ri(1, 480), '举报 r' + ri(1, 36), '公告 n' + ri(1, 30)]),
      ip: '10.12.' + ri(0, 255) + '.' + ri(1, 254),
      result: chance(0.94) ? '成功' : '失败',
      detail: '通过后台管理控制台执行「' + action[0] + '」操作。',
      createdAt: ago(ri(0, 30), ri(0, 23))
    });
  }
  LOGS.sort(function (a, b) { return b.createdAt - a.createdAt; });

  /* ========================================================================
     敏感词库
     ======================================================================== */
  const SENSITIVE_SEED = [
    ['代写代考', '违法违禁', 'high'], ['办证刷单', '广告引流', 'high'], ['高利贷', '违法违禁', 'high'],
    ['博彩下注', '违法违禁', 'high'], ['出售答案', '违法违禁', 'high'], ['加微信领资料', '广告引流', 'mid'],
    ['扫码进群', '广告引流', 'mid'], ['兼职日结', '广告引流', 'mid'], ['低价代购', '广告引流', 'mid'],
    ['私聊出售', '广告引流', 'mid'], ['点击链接', '广告引流', 'mid'], ['全场包邮', '广告引流', 'low'],
    ['傻逼', '辱骂攻击', 'high'], ['废物', '辱骂攻击', 'mid'], ['滚出去', '辱骂攻击', 'mid'],
    ['人身攻击', '辱骂攻击', 'high'], ['垃圾学校', '辱骂攻击', 'low'], ['脑残', '辱骂攻击', 'mid'],
    ['裸照', '色情低俗', 'high'], ['约炮', '色情低俗', 'high'], ['黄色网站', '色情低俗', 'high'],
    ['成人视频', '色情低俗', 'high'], ['低俗配图', '色情低俗', 'mid'], ['偷拍', '色情低俗', 'high'],
    ['造谣', '不实信息', 'mid'], ['假消息', '不实信息', 'mid'], ['内部消息', '不实信息', 'low'],
    ['保过', '不实信息', 'mid'], ['百分百通过', '不实信息', 'low'], ['官方指定', '不实信息', 'low'],
    ['身份证号', '侵犯隐私', 'high'], ['手机号出售', '侵犯隐私', 'high'], ['人肉搜索', '侵犯隐私', 'high'],
    ['开盒', '侵犯隐私', 'high'], ['曝光住址', '侵犯隐私', 'mid'], ['密码共享', '侵犯隐私', 'mid'],
    ['外挂', '违法违禁', 'mid'], ['黑客接单', '违法违禁', 'high'], ['论文代发', '违法违禁', 'high'],
    ['刷量', '广告引流', 'low']
  ];

  const SENSITIVE_WORDS = SENSITIVE_SEED.map(function (item, index) {
    return {
      id: 's' + (index + 1),
      word: item[0],
      category: item[1],
      level: item[2],
      hitCount: ri(0, 860),
      enabled: index % 11 !== 0,
      operator: pick(ADMINS).name,
      createdAt: ago(ri(10, 500))
    };
  });

  /* ========================================================================
     看板数据
     ======================================================================== */
  const userGrowth = { labels: [], newUsers: [], activeUsers: [] };
  for (let i = 29; i >= 0; i--) {
    const date = new Date(NOW - i * DAY);
    userGrowth.labels.push((date.getMonth() + 1) + '/' + date.getDate());
    const newCount = ri(18, 78);
    userGrowth.newUsers.push(newCount);
    userGrowth.activeUsers.push(ri(980, 1860));
  }

  const interaction = { labels: [], posts: [], comments: [] };
  for (let i = 13; i >= 0; i--) {
    const date = new Date(NOW - i * DAY);
    interaction.labels.push((date.getMonth() + 1) + '/' + date.getDate());
    interaction.posts.push(ri(28, 96));
    interaction.comments.push(ri(120, 420));
  }

  const RETENTION = [
    { label: '次日留存', value: 62.4, color: '#1E5BB8' },
    { label: '3 日留存', value: 41.8, color: '#3B82F6' },
    { label: '7 日留存', value: 28.6, color: '#60A5FA' },
    { label: '30 日留存', value: 15.2, color: '#93C5FD' }
  ];

  const TODOS = [
    { id: 'todo1', title: '审核 8 条待发布帖子', desc: '内容管理 · 帖子审核队列', priority: 'high', done: false, link: 'posts.html' },
    { id: 'todo2', title: '处理 5 条高优先级举报', desc: '社区治理 · 举报处理', priority: 'high', done: false, link: 'reports.html' },
    { id: 'todo3', title: '回复 3 条用户反馈', desc: '社区治理 · 用户反馈', priority: 'mid', done: false, link: 'feedbacks.html' },
    { id: 'todo4', title: '发布国庆假期值班公告', desc: '运营中心 · 公告管理', priority: 'mid', done: false, link: 'announcements.html' },
    { id: 'todo5', title: '复核 2 条封禁申诉', desc: '用户管理 · 封禁管理', priority: 'low', done: false, link: 'bans.html' },
    { id: 'todo6', title: '导出上周内容治理报表', desc: '数据统计 · 周报', priority: 'low', done: true, link: 'analytics.html' }
  ];

  const ACTIVITIES = [
    { id: 'act1', time: ago(0, 0.6), type: 'success', text: '<b>张明远</b> 审核通过了帖子《关于选课，想听听大家的意见》' },
    { id: 'act2', time: ago(0, 1.8), type: 'danger', text: '<b>李思静</b> 封禁了用户 <b>深夜的柠檬</b>，原因：发布广告引流内容' },
    { id: 'act3', time: ago(0, 3.2), type: '', text: '<b>王浩然</b> 处理了举报工单 #1032，内容已下架' },
    { id: 'act4', time: ago(1, 0.5), type: 'warning', text: '<b>陈雨欣</b> 隐藏了 3 条含违规词汇的评论' },
    { id: 'act5', time: ago(1, 4.4), type: 'success', text: '<b>刘博文</b> 发布了公告《论坛新版编辑器上线说明》' },
    { id: 'act6', time: ago(2, 2.1), type: '', text: '<b>赵雅琪</b> 更新了「考研交流」标签的运营位配置' },
    { id: 'act7', time: ago(3, 1.2), type: 'success', text: '系统完成每日数据备份，共 1,286 个用户档案' },
    { id: 'act8', time: ago(4, 3.6), type: 'warning', text: '<b>孙宇航</b> 将 2 个账号加入实名认证人工复核队列' }
  ];

  const hotTags = TAGS.slice()
    .sort(function (a, b) { return b.postCount - a.postCount || b.followCount - a.followCount; })
    .slice(0, 6)
    .map(function (tag) {
      return { id: tag.id, name: tag.name, value: tag.postCount, color: tag.color };
    });

  const STATS = {
    totalUsers: USERS.length,
    newUsersMonth: USERS.filter(function (user) { return user.registerAt >= NOW - 30 * DAY; }).length,
    newUsersWeek: USERS.filter(function (user) { return user.registerAt >= NOW - 7 * DAY; }).length,
    activeWeek: USERS.filter(function (user) { return user.status === 'active' && user.lastActiveAt >= NOW - 7 * DAY; }).length,
    bannedUsers: USERS.filter(function (user) { return user.status === 'banned'; }).length,
    mutedUsers: USERS.filter(function (user) { return user.status === 'muted'; }).length,
    pendingUsers: USERS.filter(function (user) { return user.status === 'pending'; }).length,
    totalPosts: POSTS.length,
    postsToday: POSTS.filter(function (post) { return post.createdAt >= NOW - DAY; }).length,
    pendingPosts: POSTS.filter(function (post) { return post.status === 'pending'; }).length,
    totalComments: COMMENTS.length,
    pendingReports: REPORTS.filter(function (report) { return report.status === 'pending'; }).length,
    pendingFeedbacks: FEEDBACKS.filter(function (feedback) { return feedback.status === 'pending'; }).length,
    totalAdmins: ADMINS.length
  };

  /* ========================================================================
     运营中心：消息模板 / 定向推送
     ======================================================================== */
  const NOTIFICATION_GROUPS = ['2026 级新生', '毕业生', '已认证用户', '版主团队', '高活跃用户', '沉睡用户'];

  const NOTIFICATION_TEMPLATES = [
    {
      id: 't1', name: '评论回复通知', code: 'COMMENT_REPLY', scene: '帖子收到新评论',
      channels: ['inbox', 'push'], enabled: true,
      title: '{nickname} 回复了你的帖子',
      body: '你的帖子《{postTitle}》收到新回复：\n{commentContent}\n\n点击查看完整讨论。',
      variables: ['{nickname}', '{postTitle}', '{commentContent}', '{time}']
    },
    {
      id: 't2', name: '帖子审核结果', code: 'POST_REVIEW', scene: '帖子审核完成',
      channels: ['inbox', 'email'], enabled: true,
      title: '你的帖子《{postTitle}》审核{result}',
      body: '你好 {nickname}，你于 {time} 提交的帖子《{postTitle}》已完成审核，结果：{result}。\n如有疑问可通过「意见反馈」提交申诉。',
      variables: ['{nickname}', '{postTitle}', '{result}', '{time}']
    },
    {
      id: 't3', name: '内容加精通知', code: 'POST_ESSENCE', scene: '内容被设为精华',
      channels: ['inbox', 'push'], enabled: true,
      title: '你的帖子被评为精华内容',
      body: '恭喜！你的帖子《{postTitle}》被版主设为精华，获得 {points} 积分奖励，感谢你的优质分享。',
      variables: ['{postTitle}', '{points}', '{time}']
    },
    {
      id: 't4', name: '点赞里程碑', code: 'LIKE_MILESTONE', scene: '帖子点赞达到阈值',
      channels: ['inbox'], enabled: true,
      title: '你的帖子收获 {likes} 个赞',
      body: '《{postTitle}》已获得 {likes} 个赞，继续分享有价值的内容吧。',
      variables: ['{postTitle}', '{likes}']
    },
    {
      id: 't5', name: '账号封禁通知', code: 'ACCOUNT_BAN', scene: '账号被限制登录',
      channels: ['inbox', 'email', 'push'], enabled: true,
      title: '账号处理通知：限制登录',
      body: '你好 {nickname}，因「{reason}」，你的账号已被限制登录，解封时间：{endAt}。\n如有异议可提交申诉。',
      variables: ['{nickname}', '{reason}', '{endAt}', '{operator}']
    },
    {
      id: 't6', name: '禁言通知', code: 'ACCOUNT_MUTE', scene: '账号被禁言',
      channels: ['inbox', 'email'], enabled: true,
      title: '账号处理通知：限制发言',
      body: '你好 {nickname}，因「{reason}」，你已被禁言至 {endAt}。禁言期间仍可正常浏览社区内容。',
      variables: ['{nickname}', '{reason}', '{endAt}', '{scope}']
    },
    {
      id: 't7', name: '实名认证结果', code: 'VERIFY_RESULT', scene: '实名认证审核完成',
      channels: ['inbox', 'email'], enabled: true,
      title: '实名认证审核{result}',
      body: '你好 {nickname}，你的实名认证申请已审核完成，结果：{result}。\n认证通过后可解锁正式成员权限。',
      variables: ['{nickname}', '{result}', '{time}']
    },
    {
      id: 't8', name: '积分与等级变动', code: 'POINT_CHANGE', scene: '积分达到等级门槛',
      channels: ['inbox', 'push'], enabled: false,
      title: '你已升级为「{levelName}」',
      body: '恭喜 {nickname}，你的社区积分达到 {points}，信任等级提升至 L{level}「{levelName}」。',
      variables: ['{nickname}', '{level}', '{levelName}', '{points}']
    },
    {
      id: 't9', name: '每周社区精选', code: 'WEEKLY_DIGEST', scene: '每周一 09:00 定时',
      channels: ['inbox', 'email', 'push'], enabled: true,
      title: '本周社区精选 · {weekRange}',
      body: '{nickname}，本周社区新增 {postCount} 篇优质内容，其中「{topTitle}」最受关注，点击查看完整周报。',
      variables: ['{nickname}', '{weekRange}', '{postCount}', '{topTitle}']
    },
    {
      id: 't10', name: '举报处理结果', code: 'REPORT_RESULT', scene: '举报工单处理完成',
      channels: ['inbox'], enabled: true,
      title: '你的举报已处理',
      body: '你举报的{targetType}「{targetTitle}」已完成处理，处理结果：{result}。感谢你对社区治理的支持。',
      variables: ['{targetType}', '{targetTitle}', '{result}', '{time}']
    }
  ];

  /* ========================================================================
     积分与信任等级
     ======================================================================== */
  const POINT_LEVELS = [
    { level: 0, name: '新注册用户', requirement: '完成注册即可', permission: '可浏览、发帖、评论；发帖需人工审核', mode: 'auto' },
    { level: 1, name: '正式成员', requirement: '实名认证 + 积分 ≥ 50', permission: '免审核发帖、可上传图片与附件', mode: 'auto' },
    { level: 2, name: '活跃成员', requirement: '积分 ≥ 200 且近 30 天发帖 ≥ 5', permission: '可发起投票、可创建新标签', mode: 'auto' },
    { level: 3, name: '资深成员', requirement: '积分 ≥ 800 且无违规记录', permission: '可申请版主、优质帖自主置顶', mode: 'manual' },
    { level: 4, name: '社区元老', requirement: '积分 ≥ 2000 + 管理员提名', permission: '全部成员权限 + 社区治理投票权', mode: 'manual' }
  ];

  const POINT_RULES = [
    { id: 'pr1', action: '发布帖子', value: 5, dailyCap: 20, enabled: true },
    { id: 'pr2', action: '帖子被点赞', value: 1, dailyCap: 20, enabled: true },
    { id: 'pr3', action: '发表评论', value: 2, dailyCap: 10, enabled: true },
    { id: 'pr4', action: '评论被点赞', value: 1, dailyCap: 10, enabled: true },
    { id: 'pr5', action: '帖子被收藏', value: 2, dailyCap: 20, enabled: true },
    { id: 'pr6', action: '每日首次登录', value: 1, dailyCap: 1, enabled: true },
    { id: 'pr7', action: '完成实名认证', value: 20, dailyCap: 20, enabled: true },
    { id: 'pr8', action: '举报被采纳', value: 10, dailyCap: 30, enabled: true },
    { id: 'pr9', action: '内容被设为精华', value: 50, dailyCap: 100, enabled: true },
    { id: 'pr10', action: '违规内容扣分', value: -10, dailyCap: 0, enabled: true }
  ];

  const POINT_LOG_RULES = [
    ['发布帖子', 5], ['帖子被点赞', 1], ['发表评论', 2], ['评论被点赞', 1],
    ['帖子被收藏', 2], ['每日首次登录', 1], ['完成实名认证', 20],
    ['举报被采纳', 10], ['内容被设为精华', 50], ['违规内容扣分', -10]
  ];

  const POINT_LOGS = [];
  const pointBalanceMap = {};
  for (let i = 0; i < 168; i++) {
    const user = pick(USERS);
    const rule = pick(POINT_LOG_RULES);
    if (pointBalanceMap[user.id] == null) {
      pointBalanceMap[user.id] = Math.max(rule[1] < 0 ? 60 : 0, Math.round(user.reputation * 0.62));
    }
    pointBalanceMap[user.id] = Math.max(0, pointBalanceMap[user.id] + rule[1]);
    POINT_LOGS.push({
      id: 'pl' + (i + 1),
      userId: user.id,
      studentId: user.studentId,
      nickname: user.nickname,
      action: rule[0],
      delta: rule[1],
      balance: pointBalanceMap[user.id],
      createdAt: ago(ri(0, 45), ri(0, 23))
    });
  }
  POINT_LOGS.sort(function (a, b) { return b.createdAt - a.createdAt; });

  function levelOfPoints(points) {
    if (points >= 2000) return 4;
    if (points >= 800) return 3;
    if (points >= 200) return 2;
    if (points >= 50) return 1;
    return 0;
  }

  const POINT_LEADERBOARD = USERS.slice()
    .sort(function (a, b) { return b.reputation - a.reputation; })
    .slice(0, 20)
    .map(function (user, index) {
      return {
        rank: index + 1,
        userId: user.id,
        nickname: user.nickname,
        studentId: user.studentId,
        college: user.college,
        points: user.reputation,
        level: levelOfPoints(user.reputation)
      };
    });

  /* ========================================================================
     推荐位与运营位
     ======================================================================== */
  const RECOMMENDATION_META = [
    { key: 'global', label: '全站置顶', desc: '全站帖子列表顶部固定展示' },
    { key: 'category', label: '分类置顶', desc: '各分类首屏置顶内容' },
    { key: 'carousel', label: '首页轮播', desc: '移动端首页轮播位，最多 5 条' },
    { key: 'sidebar', label: '侧边推荐', desc: '首页右侧「编辑推荐」卡片' }
  ];

  const RECOMMENDATIONS = {};
  const recommendablePosts = POSTS.filter(function (post) { return post.status === 'published'; });
  RECOMMENDATION_META.forEach(function (meta, metaIndex) {
    const count = meta.key === 'global' ? 5 : (meta.key === 'category' ? 6 : (meta.key === 'carousel' ? 4 : 5));
    const used = {};
    const list = [];
    for (let i = 0; i < count; i++) {
      let cursor = (metaIndex * 41 + i * 13) % recommendablePosts.length;
      while (used[recommendablePosts[cursor].id]) cursor = (cursor + 1) % recommendablePosts.length;
      const post = recommendablePosts[cursor];
      used[post.id] = true;
      const startAt = NOW - ri(0, 10) * DAY;
      list.push({
        id: meta.key + '-' + (i + 1),
        postId: post.id,
        title: post.title,
        authorId: post.authorId,
        authorName: post.authorName,
        board: post.board,
        tagName: post.tagName,
        viewCount: post.viewCount,
        likeCount: post.likeCount,
        commentCount: post.commentCount,
        startAt: startAt,
        endAt: startAt + ri(14, 60) * DAY,
        order: i + 1
      });
    }
    RECOMMENDATIONS[meta.key] = list;
  });

  /* ========================================================================
     数据分析（180 天日粒度，支持 7 / 30 / 90 天对比）
     ======================================================================== */
  const ANALYTICS_CATEGORIES = ['校园生活', '学习科研', '求职就业', '兴趣社团', '情感树洞'];

  const analyticsDaily = [];
  for (let i = 179; i >= 0; i--) {
    const date = new Date(NOW - i * DAY);
    analyticsDaily.push({
      date: date.getTime(),
      label: (date.getMonth() + 1) + '/' + date.getDate(),
      register: ri(14, 88),
      active: ri(920, 1980),
      posts: ri(26, 112),
      comments: ri(130, 470),
      likes: ri(180, 780)
    });
  }

  const ANALYTICS = {
    daily: analyticsDaily,
    categories: ANALYTICS_CATEGORIES.map(function (name) {
      const base = POSTS.filter(function (post) { return post.board === name; }).length;
      return {
        name: name,
        posts: base * 11 + ri(40, 160),
        comments: base * 26 + ri(120, 360),
        avgInteraction: Number((ri(180, 460) / 10).toFixed(1)),
        delta: Number(((rnd() - 0.36) * 42).toFixed(1))
      };
    }),
    retention: {
      7: { current: 38.6, previous: 35.2 },
      30: { current: 41.8, previous: 37.4 },
      90: { current: 44.2, previous: 39.8 }
    }
  };

  /* ========================================================================
     对外暴露
     ======================================================================== */
  window.MOCK_NOW = NOW;
  window.USERS = USERS;
  window.POSTS = POSTS;
  window.COMMENTS = COMMENTS;
  window.TAGS = TAGS;
  window.BANS = BANS;
  window.REPORTS = REPORTS;
  window.FEEDBACKS = FEEDBACKS;
  window.ANNOUNCEMENTS = ANNOUNCEMENTS;
  window.LOGS = LOGS;
  window.ADMINS = ADMINS;
  window.ROLES = ROLES;
  window.SENSITIVE_WORDS = SENSITIVE_WORDS;
  window.ANNOUNCEMENT_GROUPS = ANNOUNCEMENT_GROUPS;
  window.NOTIFICATION_GROUPS = NOTIFICATION_GROUPS;
  window.NOTIFICATION_TEMPLATES = NOTIFICATION_TEMPLATES;
  window.POINT_LEVELS = POINT_LEVELS;
  window.POINT_RULES = POINT_RULES;
  window.POINT_LOGS = POINT_LOGS;
  window.POINT_LEADERBOARD = POINT_LEADERBOARD;
  window.RECOMMENDATION_META = RECOMMENDATION_META;
  window.RECOMMENDATIONS = RECOMMENDATIONS;
  window.ANALYTICS = ANALYTICS;

  window.STATS = STATS;
  window.DASHBOARD = {
    growth: userGrowth,
    interaction: interaction,
    retention: RETENTION,
    todos: TODOS,
    activities: ACTIVITIES,
    hotTags: hotTags
  };

  /* 角色 / 状态 / 类型的中文映射，供各页面统一使用 */
  window.DICT = {
    userRole: { user: '普通用户', moderator: '版主', admin: '管理员' },
    userStatus: { active: '正常', muted: '禁言中', banned: '已封禁', pending: '待激活' },
    postStatus: { published: '已发布', pending: '待审核', hidden: '已隐藏', rejected: '未通过' },
    commentStatus: { normal: '正常', hidden: '已隐藏', reviewing: '复核中' },
    banType: { login: '禁止登录', post: '禁止发帖', comment: '禁止评论', all: '全站封禁' },
    banStatus: { active: '生效中', expired: '已到期', revoked: '已撤销' },
    reportStatus: { pending: '待处理', processing: '处理中', resolved: '已处理', rejected: '已驳回' },
    reportType: { post: '帖子', comment: '评论', user: '用户' },
    feedbackType: { bug: '故障反馈', feature: '功能建议', experience: '体验问题', other: '其他' },
    feedbackStatus: { pending: '待处理', processing: '处理中', resolved: '已回复' },
    announcementStatus: { draft: '草稿', published: '已发布', offline: '已下线' },
    priority: { high: '高', mid: '中', low: '低' }
  };
})();
