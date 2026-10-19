# media：音频与视频

CPU 媒体层处理 PCM、混音/播放命令、设备消费游标，以及 AVI 容器、MSV1 解码和媒体时钟。设备服务由 platform 提供，GPU 视频纹理由 scene 管理；解码和音频线程不直接修改游戏状态。

## 音频

[pcm.h](pcm.h) 拥有解码后的 PCM16 单/双声道数据；[pcm_mix.h](pcm_mix.h) 提供重采样、循环、音量和平衡。运行时不强制把原采样率改写为统一源采样率。

[audio.h](audio.h) 管理声音槽、引用计数 clip、命令版本、混音队列和实际消费游标。命令作用于尚未提交的边界，已提交音频及旧版本保持到设备消费完成。口型使用正在被消费的声音位置，不能用提交帧数或墙钟估算代替。支持保位暂停/恢复，见[音频 transport](../../reports/prop-interaction.md)。

Switch 的独立供给线程和 SDL3 输出位于 `platform/audio_switch.c` / `audio_sdl.c`，设备为 48kHz 双声道 PCM16。线程与队列语义见[性能修复](../../reports/switch-performance.md)。当前音量基准及三类默认值见[音量报告](../../reports/audio-defaults.md)，旧额外增益已撤销。

## 视频

[avi.h](avi.h)、[avi_surface.h](avi_surface.h)、[avi_clock.h](avi_clock.h) 拥有 RIFF/MSV1 字节、解码及帧时序，`scene/avi_texture` 拥有 RGBA 缓冲与 GPU 纹理。格式范围和像素政策见[AVI 报告](../../reports/avi-texture.md)；支持该子集不表示支持任意 AVI。FFmpeg 用于部分主机对照，不链接运行时。

设备播放、离线 PCM 和 GPU 纹理分别验证，证据范围见[测试说明](../../docs/testing.md)。
