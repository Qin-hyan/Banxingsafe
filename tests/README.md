# 🧪 BanSafe 测试代码

本目录存放项目的测试代码。

## 📁 目录结构

```
tests/
├── unit/              # 单元测试
│   └── test_backend.py
├── integration/       # 集成测试
└── embedded/          # 嵌入式测试
```

## 🚀 运行测试

### 单元测试
```bash
# 安装测试依赖
pip install pytest pytest-asyncio

# 运行所有单元测试
pytest tests/unit/ -v

# 运行特定测试文件
pytest tests/unit/test_backend.py -v

# 生成覆盖率报告
pytest tests/unit/ --cov=src/backend --cov-report=html
```

### 集成测试
```bash
pytest tests/integration/ -v
```

## 📊 测试覆盖率

查看测试覆盖率报告：
```bash
pytest --cov=src --cov-report=term-missing
```

## 📝 编写测试

请遵循以下规范编写测试：
1. 测试文件命名：`test_*.py`
2. 测试类命名：`Test*`
3. 测试函数命名：`test_*`
4. 使用 pytest 的 fixture 进行setUp/tearDown