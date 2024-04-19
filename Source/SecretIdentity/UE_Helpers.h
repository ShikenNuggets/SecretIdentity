// Copyright Carter Rennick, 2024. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#ifndef LOG_MSG
	#define LOG_MSG(M) if (GEngine) { GEngine->AddOnScreenDebugMessage(-1, 30.0f, FColor::Green, M); }
#endif //!LOG_MSG

#ifndef WARN_IF
	#define WARN_IF(T) if (T && GEngine) { GEngine->AddOnScreenDebugMessage(-1, 30.0f, FColor::Red, TEXT(#T " was false")); }
#endif // !WARN_IF

#ifndef WARN_IF_MSG
	#define WARN_IF_MSG(T,M) if (T && GEngine) { GEngine->AddOnScreenDebugMessage(-1, 30.0f, FColor::Red, TEXT(M)); }
#endif // !WARN_IF_MSG

#ifndef WARN_IF_NULL
	#define WARN_IF_NULL(T) if (!T && GEngine) { GEngine->AddOnScreenDebugMessage(-1, 30.0f, FColor::Red, TEXT("Required field " #T " was null")); }
#endif // !WARN_IF_NULL

/**
 * 
 */
class SECRETIDENTITY_API UE_Helpers
{
public:
	UE_Helpers();
	~UE_Helpers();
};
