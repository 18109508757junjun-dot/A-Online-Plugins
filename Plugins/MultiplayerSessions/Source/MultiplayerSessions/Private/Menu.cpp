// Fill out your copyright notice in the Description page of Project Settings.


#include "Menu.h"

void UMenu::MenuSetup() 
{
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
}
