/*
 * Tables in Shoup-style format for NTT/CT
 *
 * Input: q, n, and psi as in blzzd_table:
 * - q = psi^2 is a primitive n-th root of unity modulo q
 *
 * Output: table of powers of phi
 * - w[t + j] = phi^(n/2t)^j for t=1, 2, 4, ..., n/2
 *                          and j=0, ... t-1
 */

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>

/*
 * x^k modulo q
 */
static uint32_t power(uint32_t x, uint32_t k, uint32_t q) {
  uint32_t y;

  assert(q > 0);

  y = 1;
  while (k != 0) {
    if ((k & 1) != 0) {
      y = (y * x) % q;
    }
    k >>= 1;
    x = (x * x) % q;
  }
  return y;
}


/*
 * Check whether n is invertible modulo q and return the inverse in *inv_n
 */
static bool inverse(uint32_t n, uint32_t q, uint32_t *inv_n) {
  int32_t r1, r2, u1, u2, v1, v2, g, x;

  // invariant: r1 = n * u1 + q * v1
  //            r2 = n * u2 + q * v2
  r1 = n; u1 = 1; v1 = 0;
  r2 = q; u2 = 0, v2 = 1;
  while (r2 > 0) {
    assert(r1 == (int32_t) n * u1 + (int32_t) q * v1);
    assert(r2 == (int32_t) n * u2 + (int32_t) q * v2);
    assert(r1 >= 0);
    g = r1/r2;

    x = r1; r1 = r2; r2 = x - g * r2;
    x = u1; u1 = u2; u2 = x - g * u2;
    x = v1; v1 = v2; v2 = x - g * v2;
  }

  // r1 is gcd(n, q) = n * u1 + q * v1
  if (r1 == 1) {
    u1 = u1 % (int32_t) q;
    if (u1 < 0) u1 += q;
    assert(((((int32_t) n) * u1) % (int32_t) q) == 1);
    *inv_n = u1;
    return true;
  } else {
    return false;
  }
}


/*
 * Build the table for n, q, phi
 */
static void build_shoup_table(uint32_t *a, uint32_t n, uint32_t q, uint32_t phi) {
  uint32_t t, j, i;
  uint32_t x, y;

  a[0] = 0; // not used
  i = 1;
  for (t=1; t<n; t <<= 1) {
    x = 1;
    y = power(phi, n/(2*t), q);
    for (j=0; j<t; j++) {
      assert(i == t+j);
      assert(i < n);
      a[i] = x;
      i ++;
      x = (x * y) % q;
    }
  }
}



/*
 * Print table a:
 * - name = string to use as prefix in name<nnn>_<qqq>
 */
static void print_table(FILE *f, const char* name, uint32_t *a, uint32_t n, uint32_t q) {
  uint32_t i, k;

  k = 0;
  fprintf(f, "\nconst int32_t %s%"PRIu32"_%"PRIu32"[%"PRIu32"] = {\n", name, n, q, n);
  for (i=0; i<n; i++) {
    if (k == 0) fprintf(f, "   ");
    fprintf(f, " %5"PRIu32",", a[i]);
    k ++;
    if (k == 8) {
      fprintf(f, "\n");
      k = 0;
    }
  }
  if (k > 0) fprintf(f, "\n");
  fprintf(f, "};\n\n");
}



int main(int argc, char *argv[]) {
  uint32_t q, psi, phi, n, i, inv_n;
  uint32_t *table;
  long x;

  if (argc != 4) {
    fprintf(stderr, "Usage: %s <modulus> <size> <psi>\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  x = atol(argv[1]);
  if (x <= 1) {
    fprintf(stderr, "Invalid modulus %s: must be at least 2\n", argv[1]);
    exit(EXIT_FAILURE);    
  }
  if (x >= 0xFFFF) {
    fprintf(stderr, "The modulus is too large: max = %"PRIu32"\n", (uint32_t)0xFFFF);
    exit(EXIT_FAILURE);    
  }
  q = (uint32_t) x;

  x = atol(argv[2]);
  if (x <= 1) {
    fprintf(stderr, "Invalid size %s: must be at least 2\n", argv[2]);
    exit(EXIT_FAILURE);
  }
  if (x >= 100000) {
    fprintf(stderr, "The size is too large: max = %"PRIu32"\n", (uint32_t)100000);
    exit(EXIT_FAILURE);
  }
  n = (uint32_t) x;

  x = atol(argv[3]);
  if (x <= 1 || x >= q) {
    fprintf(stderr, "psi must be between 2 and %"PRIu32"\n", q-1);
    exit(EXIT_FAILURE);
  }
  psi = (uint32_t) x;
  phi = (psi * psi) % q;

  i = power(psi, n, q);
  if (i != q-1) {
    fprintf(stderr, "invalid psi: %"PRIu32" is not an n-th root of -1  (%"PRIu32"^n = %"PRIu32")\n", psi, psi, i);
    exit(EXIT_FAILURE);
  }
  assert(power(phi, n, q) == 1);
  for (i=1; i<n; i++) {
    if (power(psi, i, q) == 1) {
      fprintf(stderr, "invalid psi: psi^2 is not a primitive n-th root of unity (psi^2 = %"PRIu32")\n", phi);
      fprintf(stderr, "             (psi^2)^"PRIu32" = 1\n");
      exit(EXIT_FAILURE);
    }
  }
  if (!inverse(n, q, &inv_n)) {
    fprintf(stderr, "invalid parameters: %"PRIu32" is not invertible modulo %"PRIu32"\n", n, q);
    exit(EXIT_FAILURE);
  }

  fprintf(stderr, "Parameters\n");
  fprintf(stderr, "q = %"PRIu32"\n", q);
  fprintf(stderr, "n = %"PRIu32"\n", n);
  fprintf(stderr, "psi = %"PRIu32"\n", psi);
  fprintf(stderr, "psi^2 = %"PRIu32"\n", phi);
  fprintf(stderr, "n^(-1) = %"PRIu32"\n", inv_n);

  table = (uint32_t *) malloc(n * sizeof(uint32_t));
  if (table == NULL) {
    fprintf(stderr, "failed to allocate table of size %"PRIu32"\n", n);
    exit(EXIT_FAILURE);
  }
      
  build_shoup_table(table, n, q, phi);
  print_table(stdout, "shoup_ntt", table, n, q);

  free(table);

  return 0;
}

