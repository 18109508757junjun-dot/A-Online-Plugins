// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "MultiplayerSessionsSubsystem.generated.h"

/**
 * 
 */

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMultiplayerOnCreateSessionComplete, bool, bWasSuccessful);
DECLARE_MULTICAST_DELEGATE_TwoParams(FMultiplayerOnFindSessionsComplete,const TArray<FOnlineSessionSearchResult> &SessionResults, bool bWasSuccessful);
DECLARE_MULTICAST_DELEGATE_OneParam(FMultiplayerOnJoinSessionComplete, EOnJoinSessionCompleteResult::Type Result);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMultiplayerOnDestroySessionComplete, bool, bWasSuccessful);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMultiplayerOnStartSessionComplete, bool, bWasSuccessful);

UCLASS()
class MULTIPLAYERSESSIONS_API UMultiplayerSessionsSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	//多人联机子系统
	UMultiplayerSessionsSubsystem();

	void CreateSession(int32 NumPublicConnections, FString MatchType);//创建会话。接受一个连接人数和连接类型
        void FindSessions(int32 MaxSearchResults);// 查找会话。接受一个最大结果数
	void JoinSession(const FOnlineSessionSearchResult& SessionResult);//加入会话。接受一个会话搜索结果
	void StartSession();//开始会话。
	void DestroySession();//销毁会话。

	FMultiplayerOnCreateSessionComplete MultiplayerOnCreateSessionComplete;
	FMultiplayerOnFindSessionsComplete MultiplayerOnFindSessionsComplete;
    FMultiplayerOnJoinSessionComplete MultiplayerOnJoinSessionComplete;
    FMultiplayerOnDestroySessionComplete MultiplayerOnDestroySessionComplete;
    FMultiplayerOnStartSessionComplete MultiplayerOnStartSessionComplete;
	
protected:

	void OnCreateSessionComplete(FName SessionName, bool bWasSuccessful);//创建会话完成。接受一个会话名称和是否成功
	void OnFindSessionsComplete(bool bWasSuccessful);//查找会话完成。接受一个是否成功
	void OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);//加入会话完成。接受一个会话名称和加入结果
	void OnStartSessionComplete(FName SessionName, bool bWasSuccessful);//开始会话完成。接受一个会话名称和是否成功
	void OnDestroySessionComplete(FName SessionName, bool bWasSuccessful);//销毁会话完成。接受一个会话名称和是否成功

private:
    IOnlineSessionPtr SessionInterface;//在线会话接口
    TSharedPtr<FOnlineSessionSettings> LastSessionSettings; // 会话设置
    TSharedPtr<FOnlineSessionSearch> LastSessionSearch;

	FOnCreateSessionCompleteDelegate CreateSessionCompleteDelegate;
    FDelegateHandle CreateSessionCompleteDelegateHandle;
    FOnFindSessionsCompleteDelegate FindSessionsCompleteDelegate;
    FDelegateHandle FindSessionsCompleteDelegateHandle;
    FOnJoinSessionCompleteDelegate JoinSessionCompleteDelegate;
    FDelegateHandle JoinSessionCompleteDelegateHandle;
    FOnDestroySessionCompleteDelegate DestroySessionCompleteDelegate;
    FDelegateHandle DestroySessionCompleteDelegateHandle;
    FOnStartSessionCompleteDelegate StartSessionCompleteDelegate;
    FDelegateHandle StartSessionCompleteDelegateHandle;

	bool bCreateSessionOnDestroy{false};
    int32 LastNumPublicConnections;
    FString LastMatchType;
	
};
