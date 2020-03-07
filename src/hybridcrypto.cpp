#include "hybridcrypto.h"

using namespace XLib;

template <int RSAKeySize>
void HybridCrypt<RSAKeySize>::generateRSAKeys()
{
    RandomNumberGenerator rng;
    _privateKey.GenerateRandomWithKeySize(rng, RSAKeySize);
    _publicKey = RSA::PublicKey(_privateKey);
}

template <int RSAKeySize>
typename HybridCrypt<RSAKeySize>::AESData_t HybridCrypt<RSAKeySize>::generateAESKey()
{
    AutoSeededRandomPool rnd;

    rnd.GenerateBlock(_AESData.key, AESKeySize);
    rnd.GenerateBlock(_AESData.iv, AESIVSize);

    _AESData.encrypted = false;

    return _AESData;
}

template <int RSAKeySize>
bool HybridCrypt<RSAKeySize>::decryptAESKey()
{
    if (_AESData.encrypted)
    {
        auto byteAESKey = static_cast<byte*>(&_AESData);

        Integer intAESData(byteAESKey, sizeof(AESData_t));

        RandomNumberGenerator rng;

        auto decryptAESKey = _privateKey.CalculateInverse(rng, intAESData);

        decryptAESKey.Encode(byteAESKey, decryptAESKey.MinEncodedSize());

        _AESData.encrypted = false;

        return true;
    }

    return false;
}

template <int RSAKeySize>
bool HybridCrypt<RSAKeySize>::encryptAESKey()
{
    if (!_AESData.encrypted)
    {
        auto byteAESKey = static_cast<byte*>(&_AESData);

        Integer intAESData(byteAESKey, sizeof(AESData_t));

        auto ecryptAESKey = _privateKey.ApplyFunction(intAESData);

        ecryptAESKey.Encode(byteAESKey, ecryptAESKey.MinEncodedSize());

        _AESData.encrypted = true;

        return true;
    }

    return false;
}

template <int RSAKeySize>
bool HybridCrypt<RSAKeySize>::encrypt(bytes& bytes)
{
    if (!_AESData.encrypted)
    {
        CFB_Mode<AES>::Decryption cfbDecryption(_AESData.key,
                                                sizeof(_AESData.key),
                                                _AESData.iv);

        cfbDecryption.ProcessData(bytes.data(), bytes.data(), bytes.size());
        return true;
    }

    return false;
}

template <int RSAKeySize>
bool HybridCrypt<RSAKeySize>::decrypt(bytes& bytes)
{
    if (!_AESData.encrypted)
    {
        CFB_Mode<AES>::Encryption cfbEcryption(_AESData.key,
                                               sizeof(_AESData.key),
                                               _AESData.iv);

        cfbEcryption.ProcessData(bytes.data(), bytes.data(), bytes.size());
        return true;
    }

    return false;
}

template <int RSAKeySize>
typename HybridCrypt<RSAKeySize>::AESData_t HybridCrypt<RSAKeySize>::AESData() const
{
    return _AESData;
}

template <int RSAKeySize>
void HybridCrypt<RSAKeySize>::setAESData(const AESData_t& AESData_t)
{
    _AESData = AESData_t;
}

template <int RSAKeySize>
RSA::PublicKey HybridCrypt<RSAKeySize>::publicKey() const
{
    return _publicKey;
}

template <int RSAKeySize>
void HybridCrypt<RSAKeySize>::setPublicKey(const RSA::PublicKey& publicKey)
{
    _publicKey = publicKey;
}

template <int RSAKeySize>
RSA::PrivateKey HybridCrypt<RSAKeySize>::privateKey() const
{
    return _privateKey;
}

template <int RSAKeySize>
void HybridCrypt<RSAKeySize>::setPrivateKey(const RSA::PrivateKey& privateKey)
{
    _privateKey = privateKey;
}
