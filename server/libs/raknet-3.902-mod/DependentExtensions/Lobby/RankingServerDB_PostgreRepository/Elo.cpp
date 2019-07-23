#include <math.h>
#include <assert.h>

// http://en.wikipedia.org/wiki/ELO_rating_system
float CalculateNewEloRating(float playerARating, float playerBRating, float playerAScore, float playerBScore, float kFactor)
{
	assert(playerARating>=0.0f);
	assert(playerBRating>=0.0f);
	assert(playerAScore>=0.0f);
	assert(playerBScore>=0.0f);
	assert(kFactor>0.0f);

	float expectedPlayerAScore;
	float relativeScore;
	expectedPlayerAScore=(float)(1.0 / (1.0 + pow(10.0, (playerBRating-playerARating)*.0025)));
	if (playerAScore+playerBScore==0.0f)
		relativeScore=.5f; // Draw
	else
		relativeScore=playerAScore/(playerAScore+playerBScore);
	float rating = playerARating+kFactor * (relativeScore-expectedPlayerAScore);
	if (rating<0.0f)
		rating=0.0f;
	return rating;
}
