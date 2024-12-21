#include "buildcfg.h"
#include "kxbasep.h"

KXBASEAPI INT WINAPI GetUserDefaultGeoName(
	OUT	PWSTR	Buffer,
	IN	INT		BufferCch)
{
	GEOID GeoId;

	//
	// Validate parameters.
	//

	if (Buffer == NULL && BufferCch != 0) {
		RtlSetLastWin32Error(ERROR_INVALID_PARAMETER);
		return 0;
	}

	//
	// Get the user's default GeoID.
	//

	GeoId = GetUserGeoID(GEOCLASS_NATION);

	if (GeoId == GEOID_NOT_AVAILABLE) {
		GeoId = GetUserGeoID(GEOCLASS_REGION);
	}

	ASSERT (GeoId != GEOID_NOT_AVAILABLE);

	if (GeoId == GEOID_NOT_AVAILABLE) {
		RtlSetLastWin32Error(ERROR_BADDB);
		return 0;
	}

	//
	// Convert the GeoID into a two-letter ISO country code.
	//

	return GetGeoInfo(GeoId, GEO_ISO2, Buffer, BufferCch, 0);
}