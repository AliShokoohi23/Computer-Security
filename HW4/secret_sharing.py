import random
from functools import reduce

import sys
sys.setrecursionlimit(10000)

def mod_inv(a, p):
    return pow(a, p - 2, p)

def generate_polynomial(secret, t, prime):
    coefficients = [secret] + [random.randint(0, prime - 1) for _ in range(t - 1)]
    return coefficients

def evaluate_polynomial(coefficients, x, prime):
    result = 0
    for power, coeff in enumerate(coefficients):
        result = (result + coeff * pow(x, power, prime)) % prime
    return result

def split_secret(secret, n, t, prime):
    coefficients = generate_polynomial(secret, t, prime)
    shares = []
    for i in range(1, n + 1):
        x = i
        y = evaluate_polynomial(coefficients, x, prime)
        shares.append((x, y))
    return shares

def reconstruct_secret(shares, prime):
    def lagrange_interpolation(x, x_s, y_s):
        total = 0
        n = len(x_s)
        for i in range(n):
            xi, yi = x_s[i], y_s[i]
            term = yi
            for j in range(n):
                if i != j:
                    term = term * (x - x_s[j]) * mod_inv(xi - x_s[j], prime)
                    term %= prime
            total = (total + term) % prime
        return total

    x_s, y_s = zip(*shares)
    return lagrange_interpolation(0, x_s, y_s)


if __name__ == "__main__":

    secret = random.randint(0, 2**512 - 1)
    n = 10
    t = 5
    prime = 2**521 - 1

    shares = split_secret(secret, n, t, prime)
    print("Shares:")
    for share in shares:
        print(share)

    selected_shares = random.sample(shares, t+1)
    print("\nSelected Shares for Reconstruction:")
    for share in selected_shares:
        print(share)

    recovered_secret = reconstruct_secret(selected_shares, prime)
    print("\nRecovered Secret Matches Original Secret:",
          recovered_secret == secret)
