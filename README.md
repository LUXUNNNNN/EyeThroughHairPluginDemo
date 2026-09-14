# EyeThroughHairDemo

Unreal Engine 5.4 演示工程，包含 `HairEyeThrough` 渲染插件，用于演示角色眼睛透过前刘海显示的效果。

## 环境与启动

1. 安装 Unreal Engine 5.4，以及支持 UE C++ 工程的 Visual Studio 2022 和 Windows SDK。可参考根目录的 `.vsconfig` 安装组件。
2. 克隆仓库到本地。
3. 右键 `EyeThroughHairDemo.uproject`，选择 **Generate Visual Studio project files**。
4. 打开生成的解决方案，使用 **Development Editor / Win64** 编译 `EyeThroughHairDemo`。
5. 打开 `EyeThroughHairDemo.uproject`，在内容浏览器中打开 `/Game/Map/TestCleanMap`，运行演示。

首次启动需要编译项目、插件和着色器。仓库包含源码与资源，不包含预编译插件、引擎或缓存。

## 工程结构

| 路径 | 内容 |
| --- | --- |
| `Config/` | 工程默认配置 |
| `Content/` | 演示地图、材质、蓝图及相关资源 |
| `Source/` | 演示工程 C++ 模块与构建目标 |
| `Plugins/HairEyeThrough/Source/` | 插件 C++ 源码 |
| `Plugins/HairEyeThrough/Shaders/` | 眼睛与刘海合成着色器 |

`Content/__ExternalActors__` 和 `Content/__ExternalObjects__` 属于地图资源，需要随工程一起保留。

## 插件接入

将 `Plugins/HairEyeThrough` 复制到另一个 UE 5.4 C++ 工程的 `Plugins/` 目录，然后重新生成工程文件并编译。

在角色上添加 `EyeThroughHairCaptureComponent`，设置以下组件引用：

- `EyeProxyComponentRef`：参与透发显示的眼睛代理组件。
- `FrontHairComponentRef`：前刘海组件。
- `FaceComponentRef`：脸部组件。

当前代码包含 Custom Render Pass、Scene View Extension 与全屏合成着色器。深度容差、透明度及混合参数可在组件中调整。该项目为渲染原型；其他引擎版本、多视图和打包环境需要自行验证。

插件目录内的 [README](Plugins/HairEyeThrough/README.md) 记录了早期方案，其中部分接口与限制描述可能已落后于当前代码；以当前源码为准。

## 版本管理

`.gitignore` 排除 `Binaries/`、`Intermediate/`、`DerivedDataCache/`、`Saved/` 和本地 IDE 文件。`.uasset` 与 `.umap` 按二进制文件管理。

本仓库暂未添加开源许可证，第三方资源的权利归各自权利人所有。
