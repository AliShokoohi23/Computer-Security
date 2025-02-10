from P122_Util import *

def recover_key(ciphertexts, crib):
    key_length = min(len(ct) for ct in ciphertexts)
    key = [None] * key_length

    for i in range(key_length):
        frequency = {}
        for ct in ciphertexts:
            if i < len(ct):
                char = ct[i]
                frequency[char] = frequency.get(char, 0) + 1

        most_frequent_char = max(frequency, key=frequency.get)
        key[i] = chr(ord(most_frequent_char) ^ ord(crib[0]))

    return ''.join(k if k is not None else '*' for k in key)

ciphertexts = MSGS
crib = " the "

ciphertexts_bytes = [bytes.fromhex(ct) for ct in ciphertexts]
key = recover_key(ciphertexts_bytes, crib)
print("Recovered key:", key)
