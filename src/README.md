# 💻 BanSafe 源代码

本目录存放 BanSafe 项目的云端/服务端源代码。

## 📁 目录结构

```
src/
├── backend/         # 后端 API 服务
├── mobile/          # 移动端应用（Flutter）
└── README.md        # 本文件
```

## 🔧 后端服务

后端服务提供 RESTful API，处理设备通信、数据存储、警报推送等功能。

### 技术栈
- Node.js / Python
- PostgreSQL + Redis
- RabbitMQ / Kafka

### 快速开始
```bash
cd backend
npm install
npm run dev
```

## 📱 移动端应用

移动端应用供家长和责任人员使用，接收警报通知、查看设备状态等。

### 技术栈
- Flutter (iOS + Android)

### 快速开始
```bash
cd mobile
flutter pub get
flutter run
```

## 📚 相关文档

- [API 文档](../docs/api/)
- [开发指南](../docs/guides/development.md)