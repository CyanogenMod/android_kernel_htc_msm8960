/*
 * lib/reed_solomon/decode_rs.c
 *
 * Overview:
 *   Generic Reed Solomon encoder / decoder library
 *
 * Copyright 2002, Phil Karn, KA9Q
 * May be used under the terms of the GNU General Public License (GPL)
 *
 * Adaption to the kernel by Thomas Gleixner (tglx@linutronix.de)
 *
 * $Id: decode_rs.c,v 1.7 2005/11/07 11:14:59 gleixner Exp $
 *
 */

{
	int deg_lambda, el, deg_omega;
	int i, j, r, k, pad;
	int nn = rs->nn;
	int nroots = rs->nroots;
	int fcr = rs->fcr;
	int prim = rs->prim;
	int iprim = rs->iprim;
	uint16_t *alpha_to = rs->alpha_to;
	uint16_t *index_of = rs->index_of;
	uint16_t u, q, tmp, num1, num2, den, discr_r, syn_error;
	uint16_t lambda[nroots + 1], syn[nroots];
	uint16_t b[nroots + 1], t[nroots + 1], omega[nroots + 1];
	uint16_t root[nroots], reg[nroots + 1], loc[nroots];
	int count = 0;
	uint16_t msk = (uint16_t) rs->nn;

	
	pad = nn - nroots - len;
	BUG_ON(pad < 0 || pad >= nn);

	
	if (s != NULL)
		goto decode;

	for (i = 0; i < nroots; i++)
		syn[i] = (((uint16_t) data[0]) ^ invmsk) & msk;

	for (j = 1; j < len; j++) {
		for (i = 0; i < nroots; i++) {
			if (syn[i] == 0) {
				syn[i] = (((uint16_t) data[j]) ^
					  invmsk) & msk;
			} else {
				syn[i] = ((((uint16_t) data[j]) ^
					   invmsk) & msk) ^
					alpha_to[rs_modnn(rs, index_of[syn[i]] +
						       (fcr + i) * prim)];
			}
		}
	}

	for (j = 0; j < nroots; j++) {
		for (i = 0; i < nroots; i++) {
			if (syn[i] == 0) {
				syn[i] = ((uint16_t) par[j]) & msk;
			} else {
				syn[i] = (((uint16_t) par[j]) & msk) ^
					alpha_to[rs_modnn(rs, index_of[syn[i]] +
						       (fcr+i)*prim)];
			}
		}
	}
	s = syn;

	
	syn_error = 0;
	for (i = 0; i < nroots; i++) {
		syn_error |= s[i];
		s[i] = index_of[s[i]];
	}

	if (!syn_error) {
		count = 0;
		goto finish;
	}

 decode:
	memset(&lambda[1], 0, nroots * sizeof(lambda[0]));
	lambda[0] = 1;

	if (no_eras > 0) {
		
		lambda[1] = alpha_to[rs_modnn(rs,
					      prim * (nn - 1 - eras_pos[0]))];
		for (i = 1; i < no_eras; i++) {
			u = rs_modnn(rs, prim * (nn - 1 - eras_pos[i]));
			for (j = i + 1; j > 0; j--) {
				tmp = index_of[lambda[j - 1]];
				if (tmp != nn) {
					lambda[j] ^=
						alpha_to[rs_modnn(rs, u + tmp)];
				}
			}
		}
	}

	for (i = 0; i < nroots + 1; i++)
		b[i] = index_of[lambda[i]];

	r = no_eras;
	el = no_eras;
	while (++r <= nroots) {	
		
		discr_r = 0;
		for (i = 0; i < r; i++) {
			if ((lambda[i] != 0) && (s[r - i - 1] != nn)) {
				discr_r ^=
					alpha_to[rs_modnn(rs,
							  index_of[lambda[i]] +
							  s[r - i - 1])];
			}
		}
		discr_r = index_of[discr_r];	
		if (discr_r == nn) {
			
			memmove (&b[1], b, nroots * sizeof (b[0]));
			b[0] = nn;
		} else {
			
			t[0] = lambda[0];
			for (i = 0; i < nroots; i++) {
				if (b[i] != nn) {
					t[i + 1] = lambda[i + 1] ^
						alpha_to[rs_modnn(rs, discr_r +
								  b[i])];
				} else
					t[i + 1] = lambda[i + 1];
			}
			if (2 * el <= r + no_eras - 1) {
				el = r + no_eras - el;
				for (i = 0; i <= nroots; i++) {
					b[i] = (lambda[i] == 0) ? nn :
						rs_modnn(rs, index_of[lambda[i]]
							 - discr_r + nn);
				}
			} else {
				
				memmove(&b[1], b, nroots * sizeof(b[0]));
				b[0] = nn;
			}
			memcpy(lambda, t, (nroots + 1) * sizeof(t[0]));
		}
	}

	
	deg_lambda = 0;
	for (i = 0; i < nroots + 1; i++) {
		lambda[i] = index_of[lambda[i]];
		if (lambda[i] != nn)
			deg_lambda = i;
	}
	
	memcpy(&reg[1], &lambda[1], nroots * sizeof(reg[0]));
	count = 0;		
	for (i = 1, k = iprim - 1; i <= nn; i++, k = rs_modnn(rs, k + iprim)) {
		q = 1;		
		for (j = deg_lambda; j > 0; j--) {
			if (reg[j] != nn) {
				reg[j] = rs_modnn(rs, reg[j] + j);
				q ^= alpha_to[reg[j]];
			}
		}
		if (q != 0)
			continue;	
		
		root[count] = i;
		loc[count] = k;
		if (++count == deg_lambda)
			break;
	}
	if (deg_lambda != count) {
		count = -EBADMSG;
		goto finish;
	}
	deg_omega = deg_lambda - 1;
	for (i = 0; i <= deg_omega; i++) {
		tmp = 0;
		for (j = i; j >= 0; j--) {
			if ((s[i - j] != nn) && (lambda[j] != nn))
				tmp ^=
				    alpha_to[rs_modnn(rs, s[i - j] + lambda[j])];
		}
		omega[i] = index_of[tmp];
	}

	for (j = count - 1; j >= 0; j--) {
		num1 = 0;
		for (i = deg_omega; i >= 0; i--) {
			if (omega[i] != nn)
				num1 ^= alpha_to[rs_modnn(rs, omega[i] +
							i * root[j])];
		}
		num2 = alpha_to[rs_modnn(rs, root[j] * (fcr - 1) + nn)];
		den = 0;

		for (i = min(deg_lambda, nroots - 1) & ~1; i >= 0; i -= 2) {
			if (lambda[i + 1] != nn) {
				den ^= alpha_to[rs_modnn(rs, lambda[i + 1] +
						       i * root[j])];
			}
		}
		
		if (num1 != 0 && loc[j] >= pad) {
			uint16_t cor = alpha_to[rs_modnn(rs,index_of[num1] +
						       index_of[num2] +
						       nn - index_of[den])];
			if (corr) {
				corr[j] = cor;
			} else {
				if (data && (loc[j] < (nn - nroots)))
					data[loc[j] - pad] ^= cor;
			}
		}
	}

finish:
	if (eras_pos != NULL) {
		for (i = 0; i < count; i++)
			eras_pos[i] = loc[i] - pad;
	}
	return count;

}
