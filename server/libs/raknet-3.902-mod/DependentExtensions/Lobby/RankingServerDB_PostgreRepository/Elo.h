// Draws should give award a score of .5 to both players
// K factor should be 32 for low ranked/new players, 24 normally, 16 for high ranked/old players
// For high score you may wish to multiply the K Factor by some weight, to cause the score to change more rapidly.
// otherwise, long or multiple round matches will produce no more change than a short single round match.
// Games with only a win/loss (no score) should use a score of 1 for the winner, and 0 for the loser.
// All parameters must be positive
// Returns the new rating for player A
float CalculateNewEloRating(float playerARating, float playerBRating, float playerAScore, float playerBScore, float kFactor);