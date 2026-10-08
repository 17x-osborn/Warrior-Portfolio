// Fill out your copyright notice in the Description page of Project Settings.


#include "WarriorGameInstance.h"
#include "MoviePlayer.h"

void UWarriorGameInstance::OnPreLoadMap(const FString& MapName)
{
    // 创建加载屏幕属性对象
    FLoadingScreenAttributes LoadingScreenAttributes;

    // 配置加载屏幕属性
    LoadingScreenAttributes.bAutoCompleteWhenLoadingCompletes = true;
    LoadingScreenAttributes.MinimumLoadingScreenDisplayTime = 2.f;
    LoadingScreenAttributes.WidgetLoadingScreen = FLoadingScreenAttributes::NewTestLoadingScreenWidget();

    // 应用加载屏幕设置
    GetMoviePlayer()->SetupLoadingScreen(LoadingScreenAttributes);
}

void UWarriorGameInstance::OnDestinationWorldLoaded(UWorld* LoadedWorld)
{
    GetMoviePlayer()->StopMovie();
}

void UWarriorGameInstance::Init()
{
    Super::Init();

    // 预加载地图事件绑定
    FCoreUObjectDelegates::PreLoadMap.AddUObject(this, &ThisClass::OnPreLoadMap);

    // 后加载地图事件绑定
    FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &ThisClass::OnDestinationWorldLoaded);

}


TSoftObjectPtr<UWorld> UWarriorGameInstance::GetGameLevelByTag(FGameplayTag InTag) const
{
    for (const FWarriorGameLevelSet& GameLevelSet : GameLevelSets)
    {
        // 建议加上 IsNull() 检查作为补充，或者仅检查 SoftObjectPath
        if (GameLevelSet.Level.IsNull()) continue;

        if (GameLevelSet.LevelTag == InTag)
        {
            return GameLevelSet.Level;
        }
    }

    // 如果运行到这里，说明没找到 Tag
    UE_LOG(LogTemp, Warning, TEXT("GetGameLevelByTag: Failed to find level for tag %s"), *InTag.ToString());

    return TSoftObjectPtr<UWorld>();
}
