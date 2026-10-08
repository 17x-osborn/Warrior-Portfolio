// Fill out your copyright notice in the Description page of Project Settings.


#include "WFunctionLibrary.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Interfaces/PawnCombatInterface.h"
#include "AbilitySystem/WarriorAbilitySystemComponent.h"
#include "GenericTeamAgentInterface.h"
#include "Kismet/KismetMathLibrary.h"
#include "WarriorGameplayTags.h"
#include "WarriorTypes/WarriorCountDownAction.h"
#include "WarriorGameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "SaveGame/WarriorSaveGame.h"

UWarriorAbilitySystemComponent* UWFunctionLibrary::NativeGetWarriorASCFromActor(AActor* InActor)
{
    check(InActor);
    // AbilitySystemBlueprintLibrary中的函数 直接获取ASC
    return CastChecked<UWarriorAbilitySystemComponent>(UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(InActor));

}

void UWFunctionLibrary::AddGameplayTagToActorIfNone(AActor* InActor, FGameplayTag TagToAdd)
{
    UWarriorAbilitySystemComponent* ASC = NativeGetWarriorASCFromActor(InActor);
    if (!ASC->HasMatchingGameplayTag(TagToAdd)) {
        ASC->AddLooseGameplayTag(TagToAdd);
    }
}

void UWFunctionLibrary::RemoveGameplayTagFromActorIfFound(AActor* InActor, FGameplayTag TagToRemove)
{
    UWarriorAbilitySystemComponent* ASC = NativeGetWarriorASCFromActor(InActor);
    if (ASC->HasMatchingGameplayTag(TagToRemove)) {
        ASC->RemoveLooseGameplayTag(TagToRemove);
    }
}

bool UWFunctionLibrary::NativeDoesActorHaveTag(AActor* InActor, FGameplayTag TagToCheck)
{
    UWarriorAbilitySystemComponent* ASC = NativeGetWarriorASCFromActor(InActor);
    return ASC->HasMatchingGameplayTag(TagToCheck);
}

void UWFunctionLibrary::BP_DoesActorHaveTag(AActor* InActor, FGameplayTag TagToCheck, EWarriorConfirmType& OutConfirmType)
{
    OutConfirmType = NativeDoesActorHaveTag(InActor, TagToCheck) ? EWarriorConfirmType::Yes : EWarriorConfirmType::No;
}
// 获取combat组件
UPawnCombatComponent* UWFunctionLibrary::NativeGetPawnCombatComponentFromActor(AActor* InActor)
{
    check(InActor);
    if (IPawnCombatInterface* PawnCombatInterface= Cast<IPawnCombatInterface>(InActor)) {
        return PawnCombatInterface->GetPawnCombatComponent();
    }
    return nullptr;
}

UPawnCombatComponent* UWFunctionLibrary::BP_GetPawnCombatComponentFromActor(AActor* InActor, EWarriorValidType& OutValidType)
{
    UPawnCombatComponent* CombatComponent = NativeGetPawnCombatComponentFromActor(InActor);
    OutValidType = CombatComponent ? EWarriorValidType::Valid : EWarriorValidType::Invalid;
    return CombatComponent;
}

bool UWFunctionLibrary::IsTargetPawnHostile(APawn* QueryPawn, APawn* TargetPawn)
{
    check(QueryPawn && TargetPawn);
    IGenericTeamAgentInterface* QueryTeamAgent = Cast<IGenericTeamAgentInterface>(QueryPawn->GetController());
    IGenericTeamAgentInterface* TargetTeamAgent = Cast<IGenericTeamAgentInterface>(TargetPawn->GetController());

    // 如果两个Pawn都实现了团队代理接口
    if (QueryTeamAgent && TargetTeamAgent)
    {
        // 比较团队ID：如果团队ID不同，则为敌对关系
        return QueryTeamAgent->GetGenericTeamId() != TargetTeamAgent->GetGenericTeamId();
    }

    // 如果任一Pawn没有实现团队接口，默认返回非敌对（false）
    return false;
}

float UWFunctionLibrary::GetScalableFloatValueAtLevel(const FScalableFloat& InScalableFloat, float InLevel)
{
    return InScalableFloat.GetValueAtLevel(InLevel);
}

FGameplayTag UWFunctionLibrary::ComputeHitReactDirectionTag(AActor* InAttacker, AActor* InVictim, float& OutAngleDifference)
{
    check(InAttacker && InVictim);

    const FVector VictimForward = InVictim->GetActorForwardVector();
    // 正则化是把前面归1 只剩cos0
    const FVector VictimToAttackerNormalized = (InAttacker->GetActorLocation() - InVictim->GetActorLocation()).GetSafeNormal();
    // 点乘 |a||b|cos0  后面反算出0
    const float DotResult = FVector::DotProduct(VictimForward, VictimToAttackerNormalized);
    OutAngleDifference = UKismetMathLibrary::DegAcos(DotResult);

    // 计算受害者朝向向量与受害者指向攻击者向量的叉积
    const FVector CrossResult = FVector::CrossProduct(VictimForward, VictimToAttackerNormalized);
    // 在左手坐标系中(Z向上)：
    // - CrossResult.Z > 0：攻击者在受害者右侧
    // - CrossResult.Z < 0：攻击者在受害者左侧
    if (CrossResult.Z < 0.f)
    {
        OutAngleDifference *= -1.f;
    }
    if (OutAngleDifference >= -45.f && OutAngleDifference <= 45.f)
    {
        return WarriorGameplayTags::Shared_Status_HitReact_Front;
    }
    else if (OutAngleDifference < -45.f && OutAngleDifference >= -135.f)
    {
        return WarriorGameplayTags::Shared_Status_HitReact_Left;
    }
    else if (OutAngleDifference < -135.f || OutAngleDifference > 135.f)
    {
        return WarriorGameplayTags::Shared_Status_HitReact_Back;
    }
    else if (OutAngleDifference > 45.f && OutAngleDifference <= 135.f)
    {
        return WarriorGameplayTags::Shared_Status_HitReact_Right;
    }

    return WarriorGameplayTags::Shared_Status_HitReact_Front;
}

bool UWFunctionLibrary::IsValidBlock(AActor* InAttacker, AActor* InDefender)
{
    // 检查攻击者和防御者是否有效
    check(InAttacker && InDefender);

    // 计算两个角色前方向量的点积
    // > 0 : 相同方向 (夹角 < 90度)
    // = 0 : 垂直方向 (夹角 = 90度) 
    // < 0 : 相反方向 (夹角 > 90度)
    const float DotResult = FVector::DotProduct(InAttacker->GetActorForwardVector(), InDefender->GetActorForwardVector());

    // 如果点积小于0，说明两个角色面朝相反方向（面对面），攻击合法
    // 如果点积大于等于0，说明两个角色面朝相同或垂直方向，攻击不合法
    return DotResult < -0.1f;
}

bool UWFunctionLibrary::ApplyGameplayEffectSpecHandleToTargetActor(AActor* InInstigator, AActor* InTargetActor, const FGameplayEffectSpecHandle& InSpecHandle)
{
    UWarriorAbilitySystemComponent* SourceASC = NativeGetWarriorASCFromActor(InInstigator);
    UWarriorAbilitySystemComponent* TargetASC = NativeGetWarriorASCFromActor(InTargetActor);

    FActiveGameplayEffectHandle ActiveGameplayEffectHandle = SourceASC->ApplyGameplayEffectSpecToTarget(*InSpecHandle.Data, TargetASC);

    return ActiveGameplayEffectHandle.WasSuccessfullyApplied();
}

UWarriorGameInstance* UWFunctionLibrary::GetWarriorGameInstance(const UObject* WorldContextObject)
{
    if (GEngine) {
        // 从 WorldContextObject 获取对应的 UWorld 指针
        if (UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull))
        {
            // 从该世界中获取自定义的游戏实例 UMarriorGameInstance 并返回
            return World->GetGameInstance<UWarriorGameInstance>();
        }
    }

    // 如果 GEngine 不存在或获取 World 失败，则返回空指针
    return nullptr;

    
}

void UWFunctionLibrary::ToggleInputMode(const UObject* WorldContextObject, EWarriorInputMode InInputMode)
{
    // 从上下文中获取世界并取得第一个玩家控制器
    APlayerController* PlayerController = nullptr;
    if (GEngine)
    {
        // 尝试从传入的上下文对象获取游戏世界
        if (UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull))
        {
            // 获取当前世界中的第一个玩家控制器
            PlayerController = World->GetFirstPlayerController();
        }
    }
    if (!PlayerController) return;

    // 定义两种输入模式：游戏专用模式 和 UI专用模式
    FInputModeGameOnly GameOnlyMode;
    FInputModeUIOnly UIOnlyMode;

    // 根据传入的输入模式枚举值进行切换
    switch (InInputMode)
    {
    case EWarriorInputMode::GameOnly:
        // 设置输入模式为游戏专用（通常禁用鼠标光标）
        PlayerController->SetInputMode(GameOnlyMode);
        PlayerController->bShowMouseCursor = false; // 隐藏鼠标光标
        break;

    case EWarriorInputMode::UIOnly:
        // 设置输入模式为UI专用（通常允许鼠标与UI交互）
        PlayerController->SetInputMode(UIOnlyMode);
        PlayerController->bShowMouseCursor = true; // 显示鼠标光标
        break;

    default:
        break;
    }
}

void UWFunctionLibrary::SaveCurrentGameDifficulty(EWarriorGameDifficulty InDifficultyToSave)
{
    USaveGame* SaveGameObject =UGameplayStatics::CreateSaveGameObject(UWarriorSaveGame::StaticClass());

    // 将通用 USaveGame 转换为自定义的 UWarriorSaveGame
    if (UWarriorSaveGame* WarriorSaveGameObject =Cast<UWarriorSaveGame>(SaveGameObject))
    {
        // 保存当前游戏难度
        WarriorSaveGameObject->SavedCurrentGameDifficulty = InDifficultyToSave;

        // 将 SaveGame 对象写入指定的存档槽
        const bool bWasSaved= UGameplayStatics::SaveGameToSlot(WarriorSaveGameObject, WarriorGameplayTags::GameData_SaveGame_Slot_1.GetTag().ToString(), 0);
        
    }
}

bool UWFunctionLibrary::TryLoadSavedGameDifficulty(EWarriorGameDifficulty& OutSaveDifficulty)
{
    // 判断指定 Slot 的存档是否存在
    if (UGameplayStatics::DoesSaveGameExist(WarriorGameplayTags::GameData_SaveGame_Slot_1.GetTag().ToString(),0))
    {
        // 从指定 Slot 加载存档
        USaveGame* SaveGameObject =UGameplayStatics::LoadGameFromSlot(WarriorGameplayTags::GameData_SaveGame_Slot_1.GetTag().ToString(),0);

        // 转换为自定义存档类型
        if (UWarriorSaveGame* WarriorSaveGameObject =Cast<UWarriorSaveGame>(SaveGameObject))
        {
            // 从存档中读取已保存的游戏难度
            OutSaveDifficulty =WarriorSaveGameObject->SavedCurrentGameDifficulty;
            return true; 
        }
    }
    return false;

}

void UWFunctionLibrary::CountDown(const UObject* WorldContextObject, float TotalTime, float UpdateInterval, float& OutRemainingTime, EWarriorCountDownActionInput CountDownInput, UPARAM(DisplayName = "Output")EWarriorCountDownActionOutput& CountDownOutput, FLatentActionInfo LatentInfo)
{
    UWorld* World = nullptr;
    if (GEngine)
    {
        // WorldContextObject 是传入的任何 UObject（如角色、组件等）  
        World = GEngine->GetWorldFromContextObject(WorldContextObject,EGetWorldErrorMode::LogAndReturnNull);  //记录错误日志并返回空指针
    }
    if (!World)
    {
        return;  
    }
    // 获取当前世界的延迟操作管理器
    FLatentActionManager& LatentActionManager = World->GetLatentActionManager();

    // 查找是否已经存在同名的延迟操作（避免重复创建） 有的话就返回 没有在下面创建
    FWarriorCountDownAction* FoundAction = LatentActionManager.FindExistingAction<FWarriorCountDownAction>(LatentInfo.CallbackTarget,LatentInfo.UUID);

    if (CountDownInput == EWarriorCountDownActionInput::Start)
    {
        if (!FoundAction)
        {
            // 创建新的倒计时延迟操作
            LatentActionManager.AddNewAction(
                LatentInfo.CallbackTarget,    // 回调目标对象（谁创建的）
                LatentInfo.UUID,              // 唯一标识符
                new FWarriorCountDownAction(  // 创建延迟操作实例
                    TotalTime,                // 总时间
                    UpdateInterval,           // 更新间隔
                    OutRemainingTime,         // 剩余时间输出
                    CountDownOutput,          // 输出枚举控制
                    LatentInfo                // 延迟操作信息
                )
            );
        }
    }
    if (CountDownInput == EWarriorCountDownActionInput::Cancel)
    {
        if (FoundAction)
        {
            FoundAction->CancelAction();
        }
    }
}



