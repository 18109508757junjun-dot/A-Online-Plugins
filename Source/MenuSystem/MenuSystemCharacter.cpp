// Copyright Epic Games, Inc. All Rights Reserved.

#include "MenuSystemCharacter.h"
#include "Containers/AnsiString.h"
#include "Engine/LocalPlayer.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "Interfaces/OnlineSessionDelegates.h"
#include "MenuSystem.h"
#include "OnlineSubsystem.h"
#include "OnlineSessionSettings.h"
#include "Online/OnlineSessionNames.h"



AMenuSystemCharacter::AMenuSystemCharacter():
	CreateSessionCompleteDelegate(FOnCreateSessionCompleteDelegate::CreateUObject(this, &AMenuSystemCharacter::OnCreateSessionComplete)),
	FindSessionsCompleteDelegate(FOnFindSessionsCompleteDelegate::CreateUObject(this,&ThisClass::OnFindSessionsComplete)),
	JoinSessionCompleteDelegate(FOnJoinSessionCompleteDelegate::CreateUObject(this,&ThisClass::OnJoinSessionComplete))//创建会话完成委托，利用成员初始化
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);
		
	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f);

	// Note: For faster iteration times these variables, and many more, can be tweaked in the Character Blueprint
	// instead of recompiling to adjust them
	GetCharacterMovement()->JumpZVelocity = 500.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f;
	CameraBoom->bUsePawnControlRotation = true;

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named ThirdPersonCharacter (to avoid direct content references in C++)

	// set the player tag
	IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get();//获取在线子系统
	if (OnlineSubsystem)//如果在线子系统存在
	{
		OnlineSessionInterface = OnlineSubsystem->GetSessionInterface();//获取在线会话接口
		if(GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Blue, FString::Printf(TEXT("Online Subsystem found %s"), *OnlineSubsystem->GetSubsystemName().ToString()));//在屏幕上显示在线子系统的名称
		
		}
	}
}

void AMenuSystemCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent)) {
		
		// Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AMenuSystemCharacter::Move);
		EnhancedInputComponent->BindAction(MouseLookAction, ETriggerEvent::Triggered, this, &AMenuSystemCharacter::Look);

		// Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AMenuSystemCharacter::Look);
	}
	else
	{
		UE_LOG(LogMenuSystem, Error, TEXT("'%s' Failed to find an Enhanced Input component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}

void AMenuSystemCharacter::Move(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D MovementVector = Value.Get<FVector2D>();

	// route the input
	DoMove(MovementVector.X, MovementVector.Y);
}

void AMenuSystemCharacter::Look(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	// route the input
	DoLook(LookAxisVector.X, LookAxisVector.Y);
}

void AMenuSystemCharacter::DoMove(float Right, float Forward)
{
	if (GetController() != nullptr)
	{
		// find out which way is forward
		const FRotator Rotation = GetController()->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);

		// get right vector 
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		// add movement 
		AddMovementInput(ForwardDirection, Forward);
		AddMovementInput(RightDirection, Right);
	}
}

void AMenuSystemCharacter::DoLook(float Yaw, float Pitch)
{
	if (GetController() != nullptr)
	{
		// add yaw and pitch input to controller
		AddControllerYawInput(Yaw);
		AddControllerPitchInput(Pitch);
	}
}

void AMenuSystemCharacter::DoJumpStart()
{
	// signal the character to jump
	Jump();
}

void AMenuSystemCharacter::DoJumpEnd()
{
	// signal the character to stop jumping
	StopJumping();
}

//创建游戏会话(创建房间)
//学习笔记：当玩家调用这个函数，首先进行安全检查，然后将CreateSessionCompleteDelegate这个委托添加到在线会话接口中的列表中，设置session的一些属性，得到本地玩家的ID，然后调用CreateSession函数创建会话。
//这段函数执行完后，ue的online subsystem会根据设置去后台创建网络连接，分配端口等。当这些操作完成后，会调用CreateSessionCompleteDelegate委托，从而执行OnCreateSessionComplete函数。
//底层流程：1.代码发起请求 -> 2. 引擎继续逐帧渲染画面 -> 3. Steam 异步响应 -> 4. 引擎每帧轮询 (Tick) 发现响应 -> 5. 引擎底层调用 Trigger -> 6. 委托内部 for 循环遍历列表执行 Broadcast -> 7. 代码被正式唤醒并执行。
void AMenuSystemCharacter::CreateGameSession()
{
	// Host session via OnlineSessionInterface when implemented
	if(!OnlineSessionInterface.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("OnlineSessionInterface is not valid"));
		return;
	}

	auto ExistingSession = OnlineSessionInterface->GetNamedSession(NAME_GameSession);//安全检查，如果存在会话，则销毁会话。
	if(ExistingSession != nullptr)
	{
		OnlineSessionInterface->DestroySession(NAME_GameSession);
	}
	
	OnlineSessionInterface->AddOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegate);//将本地声明的委托添加到在线会话接口中的列表,以便在线会话接口调用.

	TSharedPtr<FOnlineSessionSettings> SessionSettings = MakeShareable(new FOnlineSessionSettings());//创建一个在线会话设置，F在引擎中代表c++类，makeshareable是将这个类转换为共享指针.用于安全的管理内存。
	SessionSettings->bIsLANMatch = false;//决定这个房间是局域网（LAN）游戏还是互联网游戏。
	SessionSettings->NumPublicConnections = 4;//定义房间的最大玩家容量（服务器最大承载人数）。
	SessionSettings->bUseLobbiesIfAvailable = true;//决定这个房间是否会出现在“公共服务器浏览器（Server Browser）”列表中。
	SessionSettings->bShouldAdvertise = true;//决定这个房间是否会出现在“公共服务器浏览器（Server Browser）”列表中。
	SessionSettings->bUsesPresence = true;//启用平台的“Rich Presence（富状态）”系统。什么是 Presence？当你在打游戏时，你的 Discord 或 Steam 好友列表会显示你“正在游玩 Blaster - 等待大厅 - 3/4 人”。这就是 Presence 系统的功劳。设为 true，引擎就会自动将你的 Session 状态与你的平台账号状态进行绑定并实时同步。对于现代多人游戏，这个选项几乎永远是 true。
	SessionSettings->bAllowJoinInProgress = true;//决定当游戏状态已经从“大厅（Lobby）”切换到“进行中（In-Progress）”后，是否还允许新玩家连接。
	SessionSettings->bAllowJoinViaPresence = true;//允许玩家直接通过平台的好友列表（基于 Presence）加入你的游戏。
	SessionSettings->Set(FName("MatchType"), FString("FreeForAll"), EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);//设置会话类型。比赛类型为自由对战。
	const ULocalPlayer* LocalPlayer = GetWorld()->GetFirstLocalPlayerFromController();
	OnlineSessionInterface->CreateSession(*LocalPlayer->GetPreferredUniqueNetId(), NAME_GameSession, *SessionSettings);//创建会话,参数1:玩家ID,参数2:会话名称,参数3:会话设置.
	
}

//创建会话完成回调 因为服务器有延时,不可能让客户端一直等,所以需要委托.
void AMenuSystemCharacter::OnCreateSessionComplete(FName SessionName, bool bWasSuccessful)
{
	if(bWasSuccessful)
	{
		if(GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Blue, FString::Printf(TEXT("Created session: %s"), *SessionName.ToString()));
		}

		UWorld* World = GetWorld();
		if(World)
		{
			World->ServerTravel(FString("/Game/ThirdPerson/TestMap?listen"));//服务器旅行到指定地图，listen表示服务器模式。
		}
		
	}
	else
	{
		if(GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, FString(TEXT("Failed to create session")));
		}
	}
}

void AMenuSystemCharacter::JoinGameSession()
{
	
	if(GEngine)
    {
        GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Yellow, TEXT("1. JoinGameSession Button Pressed!"));
    }
	if(!OnlineSessionInterface.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("OnlineSessionInterface is not valid"));
		return;
	}


	OnlineSessionInterface->AddOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteDelegate);//将本地声明的委托添加到在线会话接口中的列表,以便在线会话接口调用.

	SessionSearch = MakeShareable(new FOnlineSessionSearch());
	SessionSearch->MaxSearchResults = 10000;
	SessionSearch->bIsLanQuery = false;//关闭局域网
	SessionSearch->QuerySettings.Set(SEARCH_LOBBIES, true, EOnlineComparisonOp::Equals);//设置查找条件

	const ULocalPlayer* LocalPlayer = GetWorld()->GetFirstLocalPlayerFromController();
	OnlineSessionInterface->FindSessions(*LocalPlayer->GetPreferredUniqueNetId(), SessionSearch.ToSharedRef());//查找会话，参数1:玩家ID,参数2:会话搜索。
	

}

void AMenuSystemCharacter::OnFindSessionsComplete(bool bWasSuccessful)
{
	if(!OnlineSessionInterface.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("OnlineSessionInterface is not valid"));
		return;
	}
	if(!bWasSuccessful || !SessionSearch.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("FindSessions failed or SessionSearch invalid. bWasSuccessful=%d"), bWasSuccessful);
		return;
	}

	if(GEngine)
    {
        // 打印出搜索是否成功，以及搜到了多少个结果
        GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Green, FString::Printf(TEXT("2. Find Complete! Success: %d, Found Rooms: %d"), bWasSuccessful, SessionSearch->SearchResults.Num()));
    }
	for(const FOnlineSessionSearchResult& Result : SessionSearch->SearchResults)//遍历搜索结果。
	{
		FString Id = Result.GetSessionIdStr();
		FString User = Result.Session.OwningUserName;
		FString MatchType;
		Result.Session.SessionSettings.Get(FName("MatchType"), MatchType);//获取会话类型。获取比赛类型赋值给MatchType。这里获得是freeforall。
		
		if(GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Cyan, FString::Printf(TEXT("Id: %s,User: %s"), *Id, *User));
			
		}
		if(MatchType == "FreeForAll")
		{
			if(GEngine)
			{
				GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Cyan, FString::Printf(TEXT("MatchType: %s"), *MatchType));
			}

			OnlineSessionInterface->AddOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteDelegate);

			const ULocalPlayer* LocalPlayer = GetWorld() ? GetWorld()->GetFirstLocalPlayerFromController() : nullptr;//获取本地玩家。
			if(!LocalPlayer)
			{
				UE_LOG(LogTemp, Error, TEXT("LocalPlayer is null, cannot join session."));
				return;
			}
			const bool bJoinRequestAccepted = OnlineSessionInterface->JoinSession(*LocalPlayer->GetPreferredUniqueNetId(), NAME_GameSession, Result);//提交加入房间申请，result包含了房主的ip，穿透地址，自定义标签等等。
			UE_LOG(LogTemp, Log, TEXT("JoinSession requested. Accepted=%d, SessionId=%s"), bJoinRequestAccepted, *Id);
			break;
			
		}
		
	}
}

void AMenuSystemCharacter::OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
	if(!OnlineSessionInterface.IsValid())
	{
		return;
	}
	UE_LOG(LogTemp, Log, TEXT("OnJoinSessionComplete. SessionName=%s Result=%d"), *SessionName.ToString(), static_cast<int32>(Result));
	if(Result != EOnJoinSessionCompleteResult::Success)
	{
		UE_LOG(LogTemp, Warning, TEXT("Join session failed. Result=%d"), static_cast<int32>(Result));
		
		return;
	}
	FString Address;
	if(OnlineSessionInterface->GetResolvedConnectString(SessionName, Address))
	{
		/*if(GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Yellow, FString::Printf(TEXT("Connect to: %s"), *Address));
		}*/

		APlayerController* PlayerController = GetGameInstance() ? GetGameInstance()->GetFirstLocalPlayerController() : nullptr;
		if(PlayerController)
		{
			PlayerController->ClientTravel(Address, ETravelType::TRAVEL_Absolute);//客户端加入到指定地址。
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("PlayerController is null, ClientTravel skipped."));
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("GetResolvedConnectString failed. SessionName=%s"), *SessionName.ToString());
	}
}