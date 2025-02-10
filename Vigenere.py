import math
from collections import Counter

ALPHABET = 'ABCDEFGHIJKLMNOPQRSTUVWXYZ'

def index_of_coincidence(text):
    counts = Counter(text)
    total = sum(counts.values())
    ioc = sum(freq * (freq - 1) for freq in counts.values()) / (total * (total - 1))
    return 26 * ioc

def find_key_length(ciphertext, max_len=20):
    probable_lengths = []
    for key_len in range(1, max_len + 1):
        slices = [''] * key_len
        for i, char in enumerate(ciphertext):
            slices[i % key_len] += char
        avg_ioc = sum(index_of_coincidence(slice) for slice in slices) / key_len
        if avg_ioc > 1.6:
            probable_lengths.append((key_len, avg_ioc))
    return probable_lengths

def get_key(ciphertext, key_len):
    key = ''
    english_freqs = [
        0.082, 0.015, 0.028, 0.043, 0.13, 0.022, 0.02, 0.061, 0.07, 0.0015,
        0.0077, 0.04, 0.024, 0.067, 0.075, 0.019, 0.00095, 0.06, 0.063, 0.091,
        0.028, 0.0098, 0.024, 0.0015, 0.02, 0.00074
    ]
    for i in range(key_len):
        slice = ciphertext[i::key_len]
        counts = Counter(slice)
        total = sum(counts.values())
        scores = []
        for shift in range(26):
            shifted_freqs = [counts.get(ALPHABET[(j + shift) % 26], 0) / total for j in range(26)]
            scores.append(sum(f * e for f, e in zip(shifted_freqs, english_freqs)))
        key += ALPHABET[scores.index(max(scores))]
    return key

def decrypt(ciphertext, key):
    plaintext = ''
    key_indices = [ALPHABET.index(k) for k in key]
    key_pos = 0  

    for char in ciphertext:
        if char in ALPHABET:  
            shift = key_indices[key_pos % len(key)]
            plaintext += ALPHABET[(ALPHABET.index(char) - shift) % 26]
            key_pos += 1
        else:  
            plaintext += char
    return plaintext

def crack_vigenere(ciphertext):
    clean_text = ''.join(filter(str.isalpha, ciphertext.upper()))  
    probable_lengths = find_key_length(clean_text)
    if not probable_lengths:
        return "Unable to determine key length."
    
    key_length = probable_lengths[0][0]
    key = get_key(clean_text, key_length)
    plaintext = decrypt(ciphertext, key)
    return key, plaintext

ciphertext = """CLKR OADLKSYR VUBMPG GEU AX IPGVMUH WEVHOQCTSGKAX, GOMZYVEB WEIORVICX, NO- 
QMEIKR, ERITVAXENYCX, RHSPQSYTJEB EPD DLGOBIVIMEN BSSNOQMUT RI YAC LKGRPA IXJNUORVIKP KN 
DLG DOZGLYTOEXX OF DLGOBIVIMEN CYORUDIT SMMGNMI, RRYZKDSRI A PSTMKPKSKXKOX SH TRI 
EOXGGPDW OF KPIOBMVHW EPD MSOPEXCTSSP WSXJ TRI VUBMPG WEEHSRG, WRMEH MEP BO 
GONCMFEBIF A WSFEV SHA QIPEBEN-PEVROCI EOWTWTOV.VUBMPG SW YININY MSPSSH- GROH VOLI 
VHO JCTRIT OP XJEYVGTSGCL MSOPEXGR CGKEXGG.DEVKNQ XJE CIEOXH YOBPF WKV, VUBMPG GEU A 
VICDSRI PKVVIMMRAXX KN DLG BBICKSRI OP KGRWEP CSTJEBW CT LPGTMLNEI TCRU. XJE RMUTYVKAX 
EPD GETTSQG CYHGBBICKOV CSK FTIOKU HKW UASH, "AOE RGENIF EHG- GPDMONKP VAVIPT, ISW 
NOIFEN KGNSYU AD FNEDGJLOC CNN XWRSRI'S GEU TREV GORKUC." SP 8 JERG 1954, AD LKS RSWSO 
EV 43 ANPKNOXON BSCD, GMNMCPQW, DYTIXK'U HYYUEUIGPOV HOERF HSQ FEKH.CPYWV MYVVEW ACS 
RIND DLCT OZGNSRI, WRMEH NIVEBOKNOH VHKX JE REF DSIF TRI RROZKOEW FAI EV AQI 41 YIDL 
EYKRKDO TOICSPIXK EIDIF AC XJE MEWSO SH DOEVH.GLGN RMU BYHA WKW FICGQVOVGD, KR CPZPG 
LKC JAVJ-GADIP BOWKDO LKS LIF, AXH CLDLQUQL VHO ER- PVI YAC ROT DIUTOH HOB GAAXMFE, SX 
YAC WREMYNADIF TREV TRMU WKW VHO QGAXW DY GLKCR XWRSRI HKH EOXWWMOH C FKXCL NSUE"""

key, plaintext = crack_vigenere(ciphertext)
print("Key:", key)
print("Plaintext:", plaintext)
