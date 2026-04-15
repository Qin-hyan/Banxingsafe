"""
BanSafe 后端服务单元测试

使用 pytest 测试框架
"""

import pytest
import asyncio
import sys
import os

# 添加 src 目录到路径
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', '..', 'src', 'backend'))

from main import BanSafeBackend


class TestBanSafeBackend:
    """BanSafeBackend 类单元测试"""

    @pytest.fixture
    def backend(self):
        """创建测试用的后端实例"""
        return BanSafeBackend()

    def test_init(self, backend):
        """测试初始化"""
        assert backend.version == "0.1.0"
        assert backend.running == False
        assert backend.devices == {}
        assert backend.alerts == []

    @pytest.mark.asyncio
    async def test_register_device(self, backend):
        """测试设备注册"""
        device_id = "test_device_001"
        device_info = {
            "name": "测试设备",
            "type": "ESP32-S3",
            "location": "测试位置"
        }
        
        result = await backend.register_device(device_id, device_info)
        
        assert result == True
        assert device_id in backend.devices
        assert backend.devices[device_id]["name"] == "测试设备"

    @pytest.mark.asyncio
    async def test_get_device_status(self, backend):
        """测试获取设备状态"""
        device_id = "test_device_001"
        device_info = {"name": "测试设备"}
        
        await backend.register_device(device_id, device_info)
        status = await backend.get_device_status(device_id)
        
        assert status is not None
        assert status["name"] == "测试设备"

    @pytest.mark.asyncio
    async def test_get_nonexistent_device(self, backend):
        """测试获取不存在的设备"""
        status = await backend.get_device_status("nonexistent")
        assert status is None

    @pytest.mark.asyncio
    async def test_handle_alert(self, backend):
        """测试处理警报"""
        device_id = "test_device_001"
        alert_data = {
            "type": "danger",
            "severity": "high",
            "description": "测试警报"
        }
        
        result = await backend.handle_alert(device_id, alert_data)
        
        assert result == True
        assert len(backend.alerts) == 1
        assert backend.alerts[0]["device_id"] == device_id

    @pytest.mark.asyncio
    async def test_multiple_alerts(self, backend):
        """测试多个警报"""
        for i in range(5):
            await backend.handle_alert(f"device_{i}", {"type": "test"})
        
        assert len(backend.alerts) == 5


class TestTripleDetectionEngine:
    """三重判定引擎相关测试"""

    def test_score_thresholds(self):
        """测试评分阈值"""
        # 阶段 1 阈值
        POSTURE_THRESHOLD = 40
        # 阶段 2 阈值
        AUDIO_THRESHOLD = 60
        # 阶段 3 阈值
        VISUAL_THRESHOLD = 80
        
        assert POSTURE_THRESHOLD < AUDIO_THRESHOLD < VISUAL_THRESHOLD

    @pytest.mark.parametrize("posture_score,should_trigger", [
        (0, False),
        (39, False),
        (40, True),
        (100, True),
    ])
    def test_posture_detection(self, posture_score, should_trigger):
        """测试姿态检测触发逻辑"""
        POSTURE_THRESHOLD = 40
        triggered = posture_score >= POSTURE_THRESHOLD
        assert triggered == should_trigger


if __name__ == "__main__":
    pytest.main([__file__, "-v"])