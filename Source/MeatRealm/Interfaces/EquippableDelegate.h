// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

class IEquippable;

class IEquippableDelegate
{
public:
	virtual ~IEquippableDelegate() = default;
	virtual void NotifyEquippableIsExpended(IEquippable* Equippable) = 0;
};

