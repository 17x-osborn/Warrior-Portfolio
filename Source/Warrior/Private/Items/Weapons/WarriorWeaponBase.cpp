// Fill out your copyright notice in the Description page of Project Settings.


#include "Items/Weapons/WarriorWeaponBase.h"
#include "Components/BoxComponent.h"
#include "WFunctionLibrary.h"
#include "WarriorDebugHelper.h"

AWarriorWeaponBase::AWarriorWeaponBase()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	WeaponMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WeaponMesh"));
	SetRootComponent(WeaponMesh); // 设置为根组件

	WeaponCollisonBox = CreateDefaultSubobject<UBoxComponent>(TEXT("WeaponCollisonBox"));
	WeaponCollisonBox->SetupAttachment(GetRootComponent());
	WeaponCollisonBox->SetBoxExtent(FVector(20.f));
	WeaponCollisonBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	/*事件委托(OnComponentBeginOverlap)：空的信号发射器
    处理函数(OnCollisionBoxBeginOverlap)：具体的响应逻辑
    绑定(AddUniqueDynamic)：把信号发射器连接到响应逻辑,只绑定一次*/
	WeaponCollisonBox->OnComponentBeginOverlap.AddUniqueDynamic(this, &ThisClass::OnCollisionBoxBeginOverlap);
	WeaponCollisonBox->OnComponentEndOverlap.AddUniqueDynamic(this, &ThisClass::OnCollisionBoxEndOverlap);

}

// 碰撞之后获取碰撞的目标 并发送给事件
void AWarriorWeaponBase::OnCollisionBoxBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	// 获取武器的拥有者 （Instigator = 引发某个行为的责任者 等价于：APawn* WeaponOwningPawn = Cast<APawn>(GetInstigator());
	APawn* WeaponOwningPawn = GetInstigator<APawn>();
	checkf(WeaponOwningPawn, TEXT("Forgot to assign an instiagtor as the owning pawn of the weapon: %s"), *GetName());
	// 获取与武器碰撞的物体
	if (APawn* HitPawn = Cast<APawn>(OtherActor)) {
		// 确保碰撞的物体不是武器所有者自己或友方
		if (UWFunctionLibrary::IsTargetPawnHostile(WeaponOwningPawn, HitPawn)){
			//如果有人关心(绑定)武器碰撞这个事件，就告诉碰撞的目标是谁
			OnWeaponHitTarget.ExecuteIfBound(OtherActor);
		}
	}
}

void AWarriorWeaponBase::OnCollisionBoxEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	APawn* WeaponOwningPawn = GetInstigator<APawn>();
	checkf(WeaponOwningPawn, TEXT("Forgot to assign an instiagtor as the owning pawn of the weapon: %s"), *GetName());

	if (APawn* HitPawn = Cast<APawn>(OtherActor)) {
		if (UWFunctionLibrary::IsTargetPawnHostile(WeaponOwningPawn, HitPawn)) {
			//如果有人关心(绑定)武器碰撞这个事件，就告诉碰撞的目标是谁
			OnWeaponPulledFormTarget.ExecuteIfBound(OtherActor);
		}
	}
}


