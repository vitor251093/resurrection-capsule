# Darkspore.exe OpenSSL Addresses
This is the list of known OpenSSL functions inside the Darkspore.exe. We are trying to find out how the game works to make the server, and for that, the more we know about the game code, the best.

- **OTHERS..?**
- **sub_D2ECE0:** crypto/rsa/rsa_lib.c -> RSA_free
- **sub_D2EE00:** crypto/rsa/rsa_lib.c -> RSA_up_ref
- **sub_D2EE30:** ?
- **sub_D2EE50:** ?
- **sub_D2EE60:** ?
- **sub_D2EE70:** ?
- **sub_D2EE80:** ?
- **sub_D2EE90:** ?
- **sub_D2EEA0:** ?
- **sub_D2EF70:** ?
- **sub_D2F0F0:** ?
- **sub_D2F100:** ?
- **sub_D2F2F0:** ?
- **sub_D2F500:** ?
- **sub_D2F6D0:** ?
- **sub_D2F860:** ?
- **sub_D2F9B0:** ?
- **sub_D2F9C0:** ?
- **sub_D2FA10:** ?
- **sub_D2FA40:** crypto/bn/bn_lib.c -> BN_clear_free
- **OTHERS...**
- **sub_D3A0C0:** ? -> ENGINE_finish
- **OTHERS...**
- **sub_D36C80:** ? -> CRYPTO_add
- **OTHERS...**
- **sub_D38A20:** ? -> OPENSSL_free_locked
- **sub_D38A60:** ?
- **sub_D38AE0:** ?
- **sub_D38B60:** ?
- **sub_D38C20:** ? -> OPENSSL_free
- **OTHERS...**
- **sub_D3AA00:** ? -> BN_BLINDING_free
- **OTHERS..?**

## Defines
- #define REF_CHECK = FALSE
- #define REF_PRINT = FALSE
- #define CRYPTO_LOCK_RSA = 9
- #define OPENSSL_NO_ENGINE = TRUE
