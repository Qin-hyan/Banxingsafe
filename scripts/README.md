# 📜 BanSafe 构建与部署脚本

本目录存放项目的自动化脚本文件。

## 📁 脚本列表

### 构建脚本
- `build_backend.sh` / `build_backend.bat` - 后端服务构建脚本
- `build_firmware.sh` / `build_firmware.bat` - 固件构建脚本
- `build_all.sh` - 全量构建脚本

### 部署脚本
- `deploy.sh` - 生产环境部署脚本
- `deploy_staging.sh` - 测试环境部署脚本

### 工具脚本
- `setup_env.sh` - 开发环境初始化脚本
- `clean.sh` - 清理构建产物脚本
- `generate_docs.sh` - 文档生成脚本

## 💡 使用示例

```bash
# 构建后端服务
./scripts/build_backend.sh

# 构建固件
./scripts/build_firmware.sh

# 清理所有构建产物
./scripts/clean.sh
```

## 📝 注意事项

- Windows 用户请使用 `.bat` 版本的脚本
- 确保脚本有执行权限：`chmod +x scripts/*.sh`