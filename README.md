# AutoMixinEditor

## 简介

[AutoMixinEditor](file://E:\UEProject\C++\Puerts_Test\Plugins\Puerts\Source\AutoMixinEditor\AutoMixinEditor.Build.cs#L2-L26) 是一个专为 [Puerts](file://E:\UEProject\C++\Puerts_Test\Plugins\Puerts\Source\Puerts\Puerts.Build.cs#L11-L29) 设计的编辑器扩展模块，旨在提升 TypeScript 开发效率。该模块能够帮助开发者快速创建对应的 [.ts](file://E:\UEProject\C++\Puerts_Test\Plugins\Puerts\Resources\mixin.ts) 文件，并自动完成 [mixin](file://E:\UEProject\C++\Puerts_Test\Plugins\Puerts\Resources\mixin.ts#L14-L40) 绑定和模块导入操作。

## 功能特性

- 🔧 **自动创建 TypeScript 文件** - 一键生成蓝图对应的 TypeScript 绑定文件
- 🔄 **自动 mixin 绑定** - 自动生成 [mixin](file://E:\UEProject\C++\Puerts_Test\Plugins\Puerts\Resources\mixin.ts#L14-L40) 装饰器和相关代码结构
- 📦 **自动模块导入** - 自动维护 [AutoImport.ts](file://E:\UEProject\C++\Puerts_Test\TypeScript\AutoImport.ts) 文件，集中管理所有导入

## 安装说明

1. 下载完成后，直接将插件文件夹覆盖到 [Puerts](file://E:\UEProject\C++\Puerts_Test\Plugins\Puerts\Source\Puerts\Puerts.Build.cs#L11-L29) 目录中
2. 在 [Puerts.uplugin](file://E:\UEProject\C++\Puerts_Test\Plugins\Puerts\Puerts.uplugin) 文件末尾添加以下模块配置：

```json
{
    "Name": "AutoMixinEditor",
    "Type": "Editor",
    "LoadingPhase": "PostEngineInit"
}
```


## 使用说明

安装插件后，在您的入口文件（如 [MainGame.ts](file://E:\UEProject\C++\Puerts_Test\TypeScript\MainGame.ts)）中添加以下导入语句：

```typescript
import "./AutoImport";
```


这将自动导入所有通过本插件生成的 TypeScript 模块。

## 操作方式

- 在蓝图编辑器中点击工具栏的"创建TS文件"按钮
- 在内容浏览器中右键选中的蓝图，选择"创建TS文件"菜单项

## 演示视频

[B站演示视频](https://www.bilibili.com/video/BV17XVUzEEju)