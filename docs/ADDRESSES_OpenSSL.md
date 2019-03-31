# Darkspore.exe OpenSSL Addresses
This is the list of known OpenSSL functions inside the Darkspore.exe. We are trying to find out how the game works to make the server, and for that, the more we know about the game code, the best.

- **sub_D2ECE0:** crypto/rsa/rsa_lib.c -> RSA_free
- **sub_D2FA40:** crypto/bn/bn_lib.c -> BN_clear_free
- **sub_D3A0C0:** ? -> ENGINE_finish
- **sub_D36C80:** ? -> CRYPTO_add
- **sub_D38A20:** ? -> OPENSSL_free_locked
- **sub_D38C20:** ? -> OPENSSL_free
- **sub_D3AA00:** ? -> BN_BLINDING_free

## Defines
- #define REF_CHECK = FALSE
- #define REF_PRINT = FALSE
- #define CRYPTO_LOCK_RSA = 9
- #define OPENSSL_NO_ENGINE = TRUE
