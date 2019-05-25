// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

using UnrealBuildTool;
using System.Collections.Generic;

[SupportedPlatforms(UnrealPlatformClass.Server)]
public class MeatRealmServerTarget : TargetRules   // Change this line as shown previously
{
	public MeatRealmServerTarget(TargetInfo Target) : base(Target)  // Change this line as shown previously
	{
		Type = TargetType.Server;
		ExtraModuleNames.Add("MeatRealm");    // Change this line as shown previously
	}
}