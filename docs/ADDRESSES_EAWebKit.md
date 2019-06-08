# Darkspore.exe EAWebKit Addresses
This is the list of known EAWebKit functions inside the Darkspore.exe. We are trying to find out how the game works to make the server, and for that, the more we know about the game code, the best.

## ProtoSSL
- **sub_E4BC30:** `static int32_t _SendPacket(ProtoSSLRefT *pState, uint8_t uType, const void *pHeadPtr, int32_t iHeadLen, const void *pBodyPtr, int32_t iBodyLen)`

- **sub_E4DB30:** `static int32_t _ProtoSSLUpdateRecvServerCert(ProtoSSLRefT *pState, const uint8_t *pData)`
- **sub_E4DC00:** `static int32_t _ProtoSSLUpdateSendClientKey(ProtoSSLRefT *pState)`
- **sub_E4DEC0:** `static int32_t _ProtoSSLUpdateSendChange(ProtoSSLRefT *pState)`
- **sub_E4DF10:** `static int32_t _ProtoSSLUpdateSendClientFinish(ProtoSSLRefT *pState)`
- **sub_E4E0E0:** `static int32_t _ProtoSSLUpdateRecvChange(ProtoSSLRefT *pState, const uint8_t *pData)`
- **sub_E4E110:** `static int32_t _ProtoSSLUpdateRecvServerFinish(ProtoSSLRefT *pState, const uint8_t *pData)`
- **sub_E4E2F0:** `void ProtoSSLUpdate(ProtoSSLRefT *pState)`
- **sub_E4E720:** `int32_t ProtoSSLSend(ProtoSSLRefT *pState, const char *pBuffer, int32_t iLength)`
- **sub_E4E7B0:** `int32_t ProtoSSLRecv(ProtoSSLRefT *pState, char *pBuffer, int32_t iLength)`

### Socket (?)
- **sub_E54400:** `SocketSend`

### Crypt MD5
- **sub_E57310:** `CryptMD5Init`
- **sub_E57340:** `CryptMD5Update`
- **sub_E573F0:** `CryptMD5Final`

### Crypt SHA1
- **sub_E580A0:** `CryptSha1Init`
- **sub_E580E0:** `CryptSha1Update`
- **sub_E58190:** `CryptSha1Final`

## Important
If we can use Detours to modify `_ProtoSSLUpdateRecvServerCert`, in order to add `pState->bAllowAnyCert = true;` to the beginning of it, it will work without SSL.
