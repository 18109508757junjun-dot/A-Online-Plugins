// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "MultiplayerSessionsSubsystem.generated.h"

/**
 * 
 */
UCLASS()
class MULTIPLAYERSESSIONS_API UMultiplayerSessionsSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	//多人联机子系统
	UMultiplayerSessionsSubsystem();

	void CreateSession(int32 NumPublicConnections, FString MatchType);//创建会话。接受一个连接人数和连接类型
	void FindSessions(int32 MaxResults);//查找会话。接受一个最大结果数
	void JoinSession(const FOnlineSessionSearchResult& SessionResult);//加入会话。接受一个会话搜索结果
	void StartSession();//开始会话。
	void DestroySession();//销毁会话。
	
protected:

	void OnCreateSessionComplete(FName SessionName, bool bWasSuccessful);//创建会话完成。接受一个会话名称和是否成功
	void OnFindSessionsComplete(bool bWasSuccessful);//查找会话完成。接受一个是否成功
	void OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);//加入会话完成。接受一个会话名称和加入结果
	void OnStartSessionComplete(FName SessionName, bool bWasSuccessful);//开始会话完成。接受一个会话名称和是否成功
	void OnDestroySessionComplete(FName SessionName, bool bWasSuccessful);//销毁会话完成。接受一个会话名称和是否成功

private:
    IOnlineSessionPtr SessionInterface;//在线会话接口

	FOnCreateSessionCompleteDelegate CreateSessionCompleteDelegate;创建会话完成委托
	FDelegateHandle CreateSessionCompleteDelegateHandle;创建会话完成委托
	FOnFindSessionsCompleteDelegate FindSessionsCompleteDelegate;查找会话完成委托
	FDelegateHandle FindSessionsCompleteDelegateHandle;查找会话完成委托句柄
	FOnJoinSessionCompleteDelegate JoinSessionCompleteDelegate;加入会话完成委托
	FDelegateHandle JoinSessionCompleteDelegateHandle;加入会话完成委托句柄
	FOnDestroySessionCompleteDelegate DestroySessionCompleteDelegate;销毁会话完成委托
	FDelegateHandle DestroySessionCompleteDelegateHandle;销毁会话完成委托句柄
	FOnStartSessionCompleteDelegate StartSessionCompleteDelegate;开始会话完成委托
	FDelegateHandle StartSessionCompleteDelegateHandle;开始会话完成委托句柄
	
};
