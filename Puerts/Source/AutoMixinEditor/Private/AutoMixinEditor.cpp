#include "AutoMixinEditor.h"

#include "BlueprintEditorModule.h"
#include "ContentBrowserModule.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Styling/SlateStyleRegistry.h"
#include "Subsystems/EditorAssetSubsystem.h"
#include "Widgets/Notifications/SNotificationList.h"

#define LOCTEXT_NAMESPACE "FAutoMixinEditorModule"

// 常量定义
static const FString TEMPLATE_NAME = TEXT("MixinTemplate.ts"); // 模板文件名
static const FString MIXIN_NAME = TEXT("mixin.ts"); // mixin文件名
static const FString TYPE_SCRIPT_DIR = TEXT("TypeScript"); // TypeScript文件夹
static const FString PUERTS_RESOURCES_PATH = TEXT("Puerts/Resources"); // Puerts资源路径

static const FString AUTO_IMPORT_NAME = TEXT("AutoImport.ts"); // 自动导入文件名

TSharedPtr<FSlateStyleSet> FAutoMixinEditorModule::StyleSet = nullptr;

// 存储最后一个标签页
static TWeakPtr<SDockTab> LastForegroundTab = nullptr;

// 标签切换事件句柄
static FDelegateHandle TabForegroundedHandle;

// 获取AssetEditorSubsystem
static UAssetEditorSubsystem* AssetEditorSubsystem = nullptr;

// 编辑器窗口
static IAssetEditorInstance* AssetEditorInstance = nullptr;

// 最后激活的蓝图指针
static UBlueprint* LastBlueprint = nullptr;

void FAutoMixinEditorModule::StartupModule()
{
	// 获取AssetEditorSubsystem
	AssetEditorSubsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>();

	AddMixinFile(); // 添加mixin文件
	InitStyleSet(); // 初始化样式
	RegistrationButton(); // 注册按键
	RegistrationContextButton(); // 注册右键菜单

	// 订阅标签切换事件
	TabForegroundedHandle = FGlobalTabmanager::Get()->OnTabForegrounded_Subscribe(
		FOnActiveTabChanged::FDelegate::CreateLambda([](const TSharedPtr<SDockTab>& NewlyActiveTab, const TSharedPtr<SDockTab>& PreviouslyActiveTab)
		{
			if (!NewlyActiveTab.IsValid() || NewlyActiveTab == LastForegroundTab.Pin())
			{
				return;
			}
			// 不等于主要的Tab（比如EventGraph，等图标）也返回
			if (NewlyActiveTab.Get()->GetTabRole() != MajorTab) return;
			LastForegroundTab = NewlyActiveTab;
			if (LastForegroundTab.IsValid())
			{
				// UE_LOG(LogTemp, Log, TEXT("标签页已切换: %s"), *LastForegroundTab.Pin().Get()->GetTabLabel().ToString());
			}
		})
	);
}

// 注册按键
void FAutoMixinEditorModule::RegistrationButton() const
{
	// 获取蓝图编辑器模块
	FBlueprintEditorModule& BlueprintEditorModule = FModuleManager::LoadModuleChecked<FBlueprintEditorModule>("Kismet");

	// 创建一个菜单扩展器
	const TSharedPtr<FExtender> MenuExtender = MakeShareable(new FExtender());

	// 向工具栏添加扩展
	MenuExtender->AddToolBarExtension(
		"Debugging", // 扩展点名称（在调试工具栏区域）
		EExtensionHook::First, // 插入位置（最前面）
		nullptr, // 不使用命令列表
		FToolBarExtensionDelegate::CreateLambda([this](FToolBarBuilder& ToolbarBuilder)
		{
			// 添加工具栏按钮
			ToolbarBuilder.AddToolBarButton(
				FUIAction( // 按钮动作
					FExecuteAction::CreateLambda([this]() // 点击时执行的lambda
					{
						ButtonPressed(); // 调用按钮点击处理函数
					})
				),
				NAME_None, // 没有命令名
				LOCTEXT("GenerateTemplate", "创建TS文件"), // 按钮显示文本
				LOCTEXT("GenerateTemplateTooltip", "生成TypeScript文件"), // 按钮提示文本
				FSlateIcon(GetStyleName(), "MixinIcon") // 按钮图标（这里使用空图标）
			);
		})
	);

	// 将扩展器添加到蓝图编辑器的菜单扩展管理器
	BlueprintEditorModule.GetMenuExtensibilityManager()->AddExtender(MenuExtender);
}

// 在内容浏览器右键的时候注册一个按键
void FAutoMixinEditorModule::RegistrationContextButton() const
{
	FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>(TEXT("ContentBrowser"));
	TArray<FContentBrowserMenuExtender_SelectedAssets>& CBMenuAssetExtenderDelegates = ContentBrowserModule.GetAllAssetViewContextMenuExtenders();

	// 创建菜单扩展委托
	FContentBrowserMenuExtender_SelectedAssets MenuExtenderDelegate;
	MenuExtenderDelegate.BindLambda([this](const TArray<FAssetData>& SelectedAssets)
	{
		TSharedRef<FExtender> Extender = MakeShared<FExtender>();

		Extender->AddMenuExtension(
			"GetAssetActions", // 扩展菜单的锚点位置
			EExtensionHook::After, // 在指定锚点之后插入
			nullptr, // 无命令列表
			FMenuExtensionDelegate::CreateLambda([this, SelectedAssets](FMenuBuilder& MenuBuilder)
			{
				// 添加菜单项
				MenuBuilder.AddMenuEntry(
					LOCTEXT("GenerateTSFile", "创建TS文件"), // 菜单文本
					LOCTEXT("GenerateTSFileTooltip", "生成TypeScript文件"), // 提示信息
					FSlateIcon(GetStyleName(), "MixinIcon"), // 使用自定义图标
					FUIAction(
						FExecuteAction::CreateLambda([this, SelectedAssets]()
						{
							// 点击时处理选中的资源
							ContextButtonPressed(SelectedAssets);
						}),
						FCanExecuteAction::CreateLambda([SelectedAssets]()
						{
							// 可选：验证是否允许执行（例如至少选中一个资产）
							return SelectedAssets.Num() > 0;
						})
					)
				);
			})
		);
		return Extender;
	});

	// 注册扩展委托
	CBMenuAssetExtenderDelegates.Add(MenuExtenderDelegate);
}


// 当按钮被按下时调用此函数
void FAutoMixinEditorModule::ButtonPressed()
{
	// UE_LOG(LogTemp, Log, TEXT("ButtonPressed"));
	if (UBlueprint* Blueprint = GetActiveBlueprint())
	{
		GenerateTs(Blueprint);
	}
}

void FAutoMixinEditorModule::ContextButtonPressed(const TArray<FAssetData>& SelectedAssets)
{
	// 确保至少有一个资产被选中
	if (SelectedAssets.IsEmpty()) return;
	for (const FAssetData& AssetData : SelectedAssets)
	{
		// 获取选中的蓝图
		if (UBlueprint* Blueprint = Cast<UBlueprint>(AssetData.GetAsset()))
		{
			GenerateTs(Blueprint);
		}
	}
}

// 生成TS文件
void FAutoMixinEditorModule::GenerateTs(const UBlueprint* Blueprint)
{
	if (Blueprint)
	{
		// 获取蓝图的路径名称
		const FString BlueprintPath = Blueprint->GetPathName();
		FString Lefts, Rights;
		BlueprintPath.Split(".", &Lefts, &Rights);

		TArray<FString> OutStrings;
		Lefts.ParseIntoArray(OutStrings,TEXT("/"), true);

		// 实际路径
		FString ActualPath;

		if (OutStrings[0] == "Game")ActualPath = Lefts.Mid(5);
		else ActualPath = Lefts;

		// ts文件路径
		FString TsFilePath = FString(TEXT("TypeScript")) + ActualPath;
		TsFilePath = FPaths::Combine(FPaths::ProjectDir(), *(TsFilePath + ".ts"));

		// 如果ts文件不存在，则创建它
		if (!FPaths::FileExists(TsFilePath))
		{
			// 解析蓝图路径以获取文件名
			TArray<FString> StringArray;
			Rights.ParseIntoArray(StringArray,TEXT("/"), false);
			const FString FileName = StringArray[StringArray.Num() - 1];

			// 读取模板文件
			const FString TemplatePath = FPaths::Combine(FPaths::ProjectPluginsDir(), PUERTS_RESOURCES_PATH, TEMPLATE_NAME);
			FString TemplateContent;
			if (FFileHelper::LoadFileToString(TemplateContent, *TemplatePath))
			{
				// 处理模板并生成ts文件内容
				const FString TsContent = ProcessTemplate(TemplateContent, BlueprintPath, FileName);
				// 保存生成的内容到文件
				if (FFileHelper::SaveStringToFile(TsContent, *TsFilePath, FFileHelper::EEncodingOptions::ForceUTF8))
				{
					// 显示通知
					FNotificationInfo Info(FText::Format(LOCTEXT("TsFileGenerated", "TS文件生成成功->路径{0}"), FText::FromString(TsFilePath)));
					Info.ExpireDuration = 5.f;
					FSlateNotificationManager::Get().AddNotification(Info);

					// 更新AutoImport.ts文件
					const FString AutoImportTsPath = FPaths::Combine(FPaths::ProjectDir(), TYPE_SCRIPT_DIR, AUTO_IMPORT_NAME);
					FString AutoImportTsContent;

					// 读取现有内容
					if (FFileHelper::LoadFileToString(AutoImportTsContent, *AutoImportTsPath))
					{
						// 确保没有重复的导入语句
						const FString ImportStatement = TEXT("import \"./") + ActualPath.Mid(1) + "\";";
						if (!AutoImportTsContent.Contains(ImportStatement))
						{
							AutoImportTsContent += ImportStatement + TEXT("\n");
							FFileHelper::SaveStringToFile(AutoImportTsContent, *AutoImportTsPath, FFileHelper::EEncodingOptions::ForceUTF8);
							UE_LOG(LogTemp, Log, TEXT("AutoImport.ts更新成功"));
						}
					}
				}
			}
			else
			{
				// 如果模板文件不存在，记录警告
				UE_LOG(LogTemp, Warning, TEXT("MixinTemplate.ts不存在"));
			}
		}
	}
}

/**
 * @brief 获取当前激活的蓝图
 *
 * 遍历所有被编辑的资产，找到与最后前景标签匹配的活动蓝图。
 * 如果找到匹配的蓝图，则将其记录为最后激活的蓝图并返回。
 *
 * @return 当前激活的蓝图对象，如果未找到则返回nullptr
 */
UBlueprint* FAutoMixinEditorModule::GetActiveBlueprint()
{
	// 遍历所有被编辑的资产 并找到活动蓝图
	for (const TArray<UObject*> EditedAssets = AssetEditorSubsystem->GetAllEditedAssets(); UObject* EditedAsset : EditedAssets)
	{
		AssetEditorInstance = AssetEditorSubsystem->FindEditorForAsset(EditedAsset, false);

		if (!AssetEditorInstance || !IsValid(EditedAsset) || !EditedAsset->IsA<UBlueprint>()) continue;

		if (
			LastForegroundTab.Pin().Get()->GetTabLabel().ToString() ==
			AssetEditorInstance->GetAssociatedTabManager().Get()->GetOwnerTab().Get()->GetTabLabel().ToString()
		)
		{
			LastBlueprint = CastChecked<UBlueprint>(EditedAsset);
			break;
		}
	}

	// 返回最后激活的蓝图
	return LastBlueprint;
}

/**
 * @brief					处理模板
 * @param TemplateContent	模板内容 
 * @param BlueprintPath		蓝图路径
 * @param FileName			文件名
 * @return 
 */
FString FAutoMixinEditorModule::ProcessTemplate(const FString& TemplateContent, FString BlueprintPath, const FString& FileName)
{
	FString Result = TemplateContent;

	// 获取蓝图完整类名（包括_C后缀）
	BlueprintPath += TEXT("_C");
	const FString BlueprintClass = TEXT("UE") + BlueprintPath.Replace(TEXT("/"), TEXT("."));

	const FString BLUEPRINT_PATH = TEXT("BLUEPRINT_PATH"); // 蓝图路径
	const FString MIXIN_BLUEPRINT_TYPE = TEXT("MIXIN_BLUEPRINT_TYPE"); // 混入蓝图类型
	const FString TS_NAME = TEXT("TS_NAME"); // TS文件名

	Result = Result.Replace(*BLUEPRINT_PATH, *BlueprintPath); // 替换 蓝图路径
	Result = Result.Replace(*MIXIN_BLUEPRINT_TYPE, *BlueprintClass); // 替换 混入蓝图类型
	Result = Result.Replace(*TS_NAME, *FileName); // 替换 TS文件名

	return Result;
}

FName FAutoMixinEditorModule::GetStyleName()
{
	return FName(TEXT("MixinEditorStyle"));
}

/**
 * 初始化样式集合
 * 
 * 本函数负责初始化或重新初始化FAutoMixinEditorModule的样式集合
 * 它首先检查当前样式集合是否有效，如果有效，则从Slate样式注册表中注销该样式集合并重置
 * 然后，创建一个新的样式集合，设置必要的样式信息，并在Slate样式注册表中注册这个新的样式集合
 */
void FAutoMixinEditorModule::InitStyleSet()
{
	// 如果当前样式集合仍然有效，则注销并重置样式集合
	if (StyleSet.IsValid())
	{
		FSlateStyleRegistry::UnRegisterSlateStyle(*StyleSet);
		StyleSet.Reset();
	}

	// 创建一个新的样式集合，并指定其名称
	StyleSet = MakeShared<FSlateStyleSet>(GetStyleName());

	// 定义图标路径，从项目插件目录下指定子路径到达资源目录
	const FString IconPath = FPaths::Combine(FPaths::ProjectPluginsDir(), PUERTS_RESOURCES_PATH);

	// 在样式集合中设置一个名为"MixinIcon"的图标，使用指定路径下的图像文件
	StyleSet->Set("MixinIcon", new FSlateImageBrush(IconPath / "CreateFilecon.png", FVector2D(40, 40)));

	// 在Slate样式注册表中注册新的样式集合
	FSlateStyleRegistry::RegisterSlateStyle(*StyleSet);
}


void FAutoMixinEditorModule::ShutdownModule()
{
	if (StyleSet.IsValid())
	{
		FSlateStyleRegistry::UnRegisterSlateStyle(*StyleSet);
		StyleSet.Reset();
	}

	// 取消订阅标签切换事件
	FGlobalTabmanager::Get()->OnTabForegrounded_Unsubscribe(TabForegroundedHandle);
}

// 添加混入文件
void FAutoMixinEditorModule::AddMixinFile() const
{
	if (!FPaths::FileExists(MixinPath))
	{
		const FString MixinTemplatePath = FPaths::Combine(FPaths::ProjectPluginsDir(), PUERTS_RESOURCES_PATH, MIXIN_NAME);
		FString MixinContent;
		if (FFileHelper::LoadFileToString(MixinContent, *MixinTemplatePath))
		{
			FFileHelper::SaveStringToFile(MixinContent, *MixinPath, FFileHelper::EEncodingOptions::ForceUTF8);
		}
	}

	// 生成 AutoImport.ts 文件
	const FString AutoImportPath = FPaths::Combine(FPaths::ProjectDir(), TYPE_SCRIPT_DIR, AUTO_IMPORT_NAME);
	if (!FPaths::FileExists(AutoImportPath))
	{
		const FString AutoImportContent = TEXT("// 自动导入文件，用于导入所有通过Puerts创建的UE蓝图TypeScript绑定文件\n\n");
		FFileHelper::SaveStringToFile(AutoImportContent, *AutoImportPath, FFileHelper::EEncodingOptions::ForceUTF8);
	}
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FAutoMixinEditorModule, AutoMixinEditor)
