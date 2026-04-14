// Fill out your copyright notice in the Description page of Project Settings.


#include "Menu.h"
#include "Components/Button.h"
#include "MultiplayerSessionsSubsystem.h"
#include "OnlineSessionSettings.h"
#include "OnlineSubsystem.h"

void UMenu::MenuSetup(int32 NumberOfPublicConnections, FString TypeOfMatch, FString LobbyPath) 
{
    PathToLobby = FString::Printf(TEXT("%s?listen"), *LobbyPath);
    NumPublicConnections = NumberOfPublicConnections;
    MatchType = TypeOfMatch;
    AddToViewport();//将菜单添加到视口。
    SetVisibility(ESlateVisibility::Visible);//设置菜单可见。
    bIsFocusable = true;//设置菜单可聚焦。

    UWorld *World = GetWorld();
    if (World)
    {
        APlayerController *PlayerController = World->GetFirstPlayerController();
        if (PlayerController)
        {
            FInputModeUIOnly InputModeData;//输入模式数据。
            InputModeData.SetWidgetToFocus(TakeWidget());//设置焦点为菜单。
            InputModeData.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);//设置鼠标锁定行为。
            PlayerController->SetInputMode(InputModeData);//设置输入模式。
            PlayerController->SetShowMouseCursor(true);//设置鼠标可见。
        }
    }

    UGameInstance* GameInstance = GetGameInstance();
    if (GameInstance)
    {
        MultiplayerSessionsSubsystem = GameInstance->GetSubsystem<UMultiplayerSessionsSubsystem>();
    }

    if (MultiplayerSessionsSubsystem)
    {
        MultiplayerSessionsSubsystem->MultiplayerOnCreateSessionComplete.AddDynamic(this, &ThisClass::OnCreateSession);
        MultiplayerSessionsSubsystem->MultiplayerOnFindSessionsComplete.AddUObject(this, &ThisClass::OnFindSessions);
        MultiplayerSessionsSubsystem->MultiplayerOnJoinSessionComplete.AddUObject(this, &ThisClass::OnJoinSession);
        MultiplayerSessionsSubsystem->MultiplayerOnDestroySessionComplete.AddDynamic(this,&ThisClass::OnDestroySession);
        MultiplayerSessionsSubsystem->MultiplayerOnStartSessionComplete.AddDynamic(this, &ThisClass::OnStartSession);

    }

}

bool UMenu::Initialize() 
{ 
    if (!Super::Initialize())
    {
        return false;
    }
    
    if (HostButton)
    {
        HostButton->OnClicked.AddDynamic(this, &ThisClass::HostButtonClicked);
    }
    if (JoinButton)
    {
        JoinButton->OnClicked.AddDynamic(this, &ThisClass::JoinButtonClicked);
    }

    return true; 
}

void UMenu::NativeDestruct() 
{
    MenuTearDown();
    Super::NativeDestruct();
}

void UMenu::OnCreateSession(bool bWasSuccessful) 
{
    if (bWasSuccessful)
    {
        if (GEngine)
        {
            GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Yellow, FString(TEXT("Session created successfully!")));
        }

        UWorld *World = GetWorld();
        if (World)
        {
            World->ServerTravel(PathToLobby); // 服务器旅行到指定地图，listen表示服务器模式。
        }

        else
        {
            if (GEngine)
            {
                GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Red, FString(TEXT("Failed to create session!")));
            }
            HostButton->SetIsEnabled(true);
        }


    }
}

void UMenu::OnFindSessions(const TArray<FOnlineSessionSearchResult> &SessionResults, bool bWasSuccessful) 
{
    if (MultiplayerSessionsSubsystem == nullptr)
    {
        return;
    }

    for (auto Result : SessionResults)
    {
        FString SettingsValue;
        Result.Session.SessionSettings.Get(FName("MatchType"), SettingsValue);
        if (SettingsValue == MatchType)
        {
            MultiplayerSessionsSubsystem->JoinSession(Result);
            return;
        }
    }
    
    if (!bWasSuccessful || SessionResults.Num() == 0)
    {
        JoinButton->SetIsEnabled(true);
    }
}

void UMenu::OnJoinSession(EOnJoinSessionCompleteResult::Type Result) 
{
    IOnlineSubsystem *Subsystem = IOnlineSubsystem::Get();
    if (Subsystem)
    {
        IOnlineSessionPtr SessionInterface = Subsystem->GetSessionInterface();
        if (SessionInterface.IsValid())
        {
            FString Address;
            SessionInterface->GetResolvedConnectString(NAME_GameSession, Address);

            APlayerController *PlayerController = GetGameInstance()->GetFirstLocalPlayerController();
            if (PlayerController)
            {
                PlayerController->ClientTravel(Address, ETravelType::TRAVEL_Absolute);
            }
        }
    }
    if (Result != EOnJoinSessionCompleteResult::Success)
    {
        JoinButton->SetIsEnabled(true);
    }
}



void UMenu::OnDestroySession(bool bWasSuccessful) {}

void UMenu::OnStartSession(bool bWasSuccessful) {}

void UMenu::HostButtonClicked() 
{
    HostButton->SetIsEnabled(false);

    if (MultiplayerSessionsSubsystem)
    {
        MultiplayerSessionsSubsystem->CreateSession(NumPublicConnections, MatchType);
        
	}
}

void UMenu::JoinButtonClicked() 
{ 
    JoinButton->SetIsEnabled(false);

    if (MultiplayerSessionsSubsystem)
    {
        MultiplayerSessionsSubsystem->FindSessions(10000);
    }
}

void UMenu::MenuTearDown()
{
	RemoveFromParent();
	UWorld* World = GetWorld();
	if (World)
	{
		APlayerController* PlayerController = World->GetFirstPlayerController();
		if (PlayerController)
		{
			FInputModeGameOnly InputModeData;
			PlayerController->SetInputMode(InputModeData);
			PlayerController->SetShowMouseCursor(false);
		}
	}
}