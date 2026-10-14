// Alex Goldberg rights reserved

#pragma once

UENUM()
enum class EGodsentConfirmType : uint8
{
	Yes,
	No
};

UENUM()
enum class EGodsentValidType : uint8
{
	Valid,
	Invalid
};

UENUM()
enum class EGodsentSuccessType : uint8
{
	Successful,
	Failed
};

UENUM()
enum class EGodsentCountDownActionInput : uint8
{
	Start,
	Cancel
};

UENUM()
enum class EGodsentCountDownActionOutput : uint8
{
	Updated,
	Completed,
	Cancelled
};
