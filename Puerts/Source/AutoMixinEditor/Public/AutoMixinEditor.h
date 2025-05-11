﻿#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FAutoMixinEditorModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	// 添加mixin文件
	void AddMixinFile() const;
	// 注册按键
	void RegistrationButton() const;
	// 按键按下
	static void ButtonPressed();
	// 获取当前激活的Blueprint
	static UBlueprint* GetActiveBlueprint();

	// 在内容浏览器右键注册一个按键
	void RegistrationContextButton() const;

	/**
	 * @brief 菜单按钮按下
	 * @param SelectedAssets	选中的资产 
	 */
	static void ContextButtonPressed(const TArray<FAssetData>& SelectedAssets);

	// 生成ts文件
	static void GenerateTs(const UBlueprint* Blueprint);

	/**
	 * 处理模板
	 * @param TemplateContent	模板内容 
	 * @param BlueprintPath		蓝图路径路径
	 * @param FileName			文件名
	 * @return 
	 */
	static FString ProcessTemplate(const FString& TemplateContent, FString BlueprintPath, const FString& FileName);

	// 样式名称
	static FName GetStyleName();

	/**
	* 初始化样式集合
	* 
	* 本函数负责初始化或重新初始化FAutoMixinEditorModule的样式集合
	* 它首先检查当前样式集合是否有效，如果有效，则从Slate样式注册表中注销该样式集合并重置
	 * 然后，创建一个新的样式集合，设置必要的样式信息，并在Slate样式注册表中注册这个新的样式集合
	*/
	static void InitStyleSet();

	// 样式
	static TSharedPtr<FSlateStyleSet> StyleSet;

	// mixin文件路径
	const FString MixinPath = FPaths::Combine(FPaths::ProjectDir(),TEXT("TypeScript"),TEXT("mixin.ts"));
};
