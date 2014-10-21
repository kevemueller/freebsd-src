/*! \file ssl/ssl_lib.c
 *  \brief Version independent SSL functions.
 */
/* Copyright (C) 1995-1998 Eric Young (eay@cryptsoft.com)
 * All rights reserved.
 *
 * This package is an SSL implementation written
 * by Eric Young (eay@cryptsoft.com).
 * The implementation was written so as to conform with Netscapes SSL.
 * 
 * This library is free for commercial and non-commercial use as long as
 * the following conditions are aheared to.  The following conditions
 * apply to all code found in this distribution, be it the RC4, RSA,
 * lhash, DES, etc., code; not just the SSL code.  The SSL documentation
 * included with this distribution is covered by the same copyright terms
 * except that the holder is Tim Hudson (tjh@cryptsoft.com).
 * 
 * Copyright remains Eric Young's, and as such any Copyright notices in
 * the code are not to be removed.
 * If this package is used in a product, Eric Young should be given attribution
 * as the author of the parts of the library used.
 * This can be in the form of a textual message at program startup or
 * in documentation (online or textual) provided with the package.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *    "This product includes cryptographic software written by
 *     Eric Young (eay@cryptsoft.com)"
 *    The word 'cryptographic' can be left out if the rouines from the library
 *    being used are not cryptographic related :-).
 * 4. If you include any Windows specific code (or a derivative thereof) from 
 *    the apps directory (application code) you must include an acknowledgement:
 *    "This product includes software written by Tim Hudson (tjh@cryptsoft.com)"
 * 
 * THIS SOFTWARE IS PROVIDED BY ERIC YOUNG ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 * 
 * The licence and distribution terms for any publically available version or
 * derivative of this code cannot be changed.  i.e. this code cannot simply be
 * copied and put under another distribution licence
 * [including the GNU Public Licence.]
 */
/* ====================================================================
 * Copyright (c) 1998-2001 The OpenSSL Project.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer. 
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * 3. All advertising materials mentioning features or use of this
 *    software must display the following acknowledgment:
 *    "This product includes software developed by the OpenSSL Project
 *    for use in the OpenSSL Toolkit. (http://www.openssl.org/)"
 *
 * 4. The names "OpenSSL Toolkit" and "OpenSSL Project" must not be used to
 *    endorse or promote products derived from this software without
 *    prior written permission. For written permission, please contact
 *    openssl-core@openssl.org.
 *
 * 5. Products derived from this software may not be called "OpenSSL"
 *    nor may "OpenSSL" appear in their names without prior written
 *    permission of the OpenSSL Project.
 *
 * 6. Redistributions of any form whatsoever must retain the following
 *    acknowledgment:
 *    "This product includes software developed by the OpenSSL Project
 *    for use in the OpenSSL Toolkit (http://www.openssl.org/)"
 *
 * THIS SOFTWARE IS PROVIDED BY THE OpenSSL PROJECT ``AS IS'' AND ANY
 * EXPRESSED OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE OpenSSL PROJECT OR
 * ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 * ====================================================================
 *
 * This product includes cryptographic software written by Eric Young
 * (eay@cryptsoft.com).  This product includes software written by Tim
 * Hudson (tjh@cryptsoft.com).
 *
 */
/* ====================================================================
 * Copyright 2002 Sun Microsystems, Inc. ALL RIGHTS RESERVED.
 * ECC cipher suite support in OpenSSL originally developed by 
 * SUN MICROSYSTEMS, INC., and contributed to the OpenSSL project.
 */

#ifdef REF_CHECK
#  include <assert.h>
#endif
#include <stdio.h>
#include "ssl_locl.h"
#include "kssl_lcl.h"
#include <openssl/objects.h>
#include <openssl/lhash.h>
#include <openssl/x509v3.h>
#include <openssl/rand.h>
#include <openssl/ocsp.h>
#ifndef OPENSSL_NO_DH
#include <openssl/dh.h>
#endif
#ifndef OPENSSL_NO_ENGINE
#include <openssl/engine.h>
#endif

const char *SSL_version_str=OPENSSL_VERSION_TEXT;

SSL3_ENC_METHOD ssl3_undef_enc_method={
	/* evil casts, but these functions are only called if there's a library bug */
	(int (*)(SSL *,int))ssl_undefined_function,
	(int (*)(SSL *, unsigned char *, int))ssl_undefined_function,
	ssl_undefined_function,
	(int (*)(SSL *, unsigned char *, unsigned char *, int))ssl_undefined_function,
	(int (*)(SSL*, int))ssl_undefined_function,
	(int (*)(SSL *, EVP_MD_CTX *, EVP_MD_CTX *, const char*, int, unsigned char *))ssl_undefined_function,
	0,	/* finish_mac_length */
	(int (*)(SSL *, EVP_MD_CTX *, unsigned char *))ssl_undefined_function,
	NULL,	/* client_finished_label */
	0,	/* client_finished_label_len */
	NULL,	/* server_finished_label */
	0,	/* server_finished_label_len */
	(int (*)(int))ssl_undefined_function
	};

int SSL_clear(SSL *s)
	{

	if (s->method == NULL)
		{
		SSLerr(SSL_F_SSL_CLEAR,SSL_R_NO_METHOD_SPECIFIED);
		return(0);
		}

	if (ssl_clear_bad_session(s))
		{
		SSL_SESSION_free(s->session);
		s->session=NULL;
		}

	s->error=0;
	s->hit=0;
	s->shutdown=0;

#if 0 /* Disabled since version 1.10 of this file (early return not
       * needed because SSL_clear is not called when doing renegotiation) */
	/* This is set if we are doing dynamic renegotiation so keep
	 * the old cipher.  It is sort of a SSL_clear_lite :-) */
	if (s->new_session) return(1);
#else
	if (s->new_session)
		{
		SSLerr(SSL_F_SSL_CLEAR,ERR_R_INTERNAL_ERROR);
		return 0;
		}
#endif

	s->type=0;

	s->state=SSL_ST_BEFORE|((s->server)?SSL_ST_ACCEPT:SSL_ST_CONNECT);

	s->version=s->method->version;
	s->client_version=s->version;
	s->rwstate=SSL_NOTHING;
	s->rstate=SSL_ST_READ_HEADER;
#if 0
	s->read_ahead=s->ctx->read_ahead;
#endif

	if (s->init_buf != NULL)
		{
		BUF_MEM_free(s->init_buf);
		s->init_buf=NULL;
		}

	ssl_clear_cipher_ctx(s);

	s->first_packet=0;

#if 1
	/* Check to see if we were changed into a different method, if
	 * so, revert back if we are not doing session-id reuse. */
	if (!s->in_handshake && (s->session == NULL) && (s->method != s->ctx->method))
		{
		s->method->ssl_free(s);
		s->method=s->ctx->method;
		if (!s->method->ssl_new(s))
			return(0);
		}
	else
#endif
		s->method->ssl_clear(s);
	return(1);
	}

/** Used to change an SSL_CTXs default SSL method type */
int SSL_CTX_set_ssl_version(SSL_CTX *ctx,SSL_METHOD *meth)
	{
	STACK_OF(SSL_CIPHER) *sk;

	ctx->method=meth;

	sk=ssl_create_cipher_list(ctx->method,&(ctx->cipher_list),
		&(ctx->cipher_list_by_id),SSL_DEFAULT_CIPHER_LIST);
	if ((sk == NULL) || (sk_SSL_CIPHER_num(sk) <= 0))
		{
		SSLerr(SSL_F_SSL_CTX_SET_SSL_VERSION,SSL_R_SSL_LIBRARY_HAS_NO_CIPHERS);
		return(0);
		}
	return(1);
	}

SSL *SSL_new(SSL_CTX *ctx)
	{
	SSL *s;

	if (ctx == NULL)
		{
		SSLerr(SSL_F_SSL_NEW,SSL_R_NULL_SSL_CTX);
		return(NULL);
		}
	if (ctx->method == NULL)
		{
		SSLerr(SSL_F_SSL_NEW,SSL_R_SSL_CTX_HAS_NO_DEFAULT_SSL_VERSION);
		return(NULL);
		}

	s=(SSL *)OPENSSL_malloc(sizeof(SSL));
	if (s == NULL) goto err;
	memset(s,0,sizeof(SSL));

#ifndef	OPENSSL_NO_KRB5
	s->kssl_ctx = kssl_ctx_new();
#endif	/* OPENSSL_NO_KRB5 */

	s->options=ctx->options;
	s->mode=ctx->mode;
	s->max_cert_list=ctx->max_cert_list;

	if (ctx->cert != NULL)
		{
		/* Earlier library versions used to copy the pointer to
		 * the CERT, not its contents; only when setting new
		 * parameters for the per-SSL copy, ssl_cert_new would be
		 * called (and the direct reference to the per-SSL_CTX
		 * settings would be lost, but those still were indirectly
		 * accessed for various purposes, and for that reason they
		 * used to be known as s->ctx->default_cert).
		 * Now we don't look at the SSL_CTX's CERT after having
		 * duplicated it once. */

		s->cert = ssl_cert_dup(ctx->cert);
		if (s->cert == NULL)
			goto err;
		}
	else
		s->cert=NULL; /* Cannot really happen (see SSL_CTX_new) */

	s->read_ahead=ctx->read_ahead;
	s->msg_callback=ctx->msg_callback;
	s->msg_callback_arg=ctx->msg_callback_arg;
	s->verify_mode=ctx->verify_mode;
#if 0
	s->verify_depth=ctx->verify_depth;
#endif
	s->sid_ctx_length=ctx->sid_ctx_length;
	OPENSSL_assert(s->sid_ctx_length <= sizeof s->sid_ctx);
	memcpy(&s->sid_ctx,&ctx->sid_ctx,sizeof(s->sid_ctx));
	s->verify_callback=ctx->default_verify_callback;
	s->generate_session_id=ctx->generate_session_id;

	s->param = X509_VERIFY_PARAM_new();
	if (!s->param)
		goto err;
	X509_VERIFY_PARAM_inherit(s->param, ctx->param);
#if 0
	s->purpose = ctx->purpose;
	s->trust = ctx->trust;
#endif
	s->quiet_shutdown=ctx->quiet_shutdown;

	CRYPTO_add(&ctx->references,1,CRYPTO_LOCK_SSL_CTX);
	s->ctx=ctx;
#ifndef OPENSSL_NO_TLSEXT
	s->tlsext_debug_cb = 0;
	s->tlsext_debug_arg = NULL;
	s->tlsext_ticket_expected = 0;
	s->tlsext_status_type = -1;
	s->tlsext_status_expected = 0;
	s->tlsext_ocsp_ids = NULL;
	s->tlsext_ocsp_exts = NULL;
	s->tlsext_ocsp_resp = NULL;
	s->tlsext_ocsp_resplen = -1;
	CRYPTO_add(&ctx->references,1,CRYPTO_LOCK_SSL_CTX);
	s->initial_ctx=ctx;
#endif
	s->verify_result=X509_V_OK;

	s->method=ctx->method;

	if (!s->method->ssl_new(s))
		goto err;

	s->references=1;
	s->server=(ctx->method->ssl_accept == ssl_undefined_function)?0:1;

	SSL_clear(s);

	CRYPTO_new_ex_data(CRYPTO_EX_INDEX_SSL, s, &s->ex_data);

	return(s);
err:
	if (s != NULL)
		{
		if (s->cert != NULL)
			ssl_cert_free(s->cert);
		if (s->ctx != NULL)
			SSL_CTX_free(s->ctx); /* decrement reference count */
		OPENSSL_free(s);
		}
	SSLerr(SSL_F_SSL_NEW,ERR_R_MALLOC_FAILURE);
	return(NULL);
	}

int SSL_CTX_set_session_id_context(SSL_CTX *ctx,const unsigned char *sid_ctx,
				   unsigned int sid_ctx_len)
    {
    if(sid_ctx_len > sizeof ctx->sid_ctx)
	{
	SSLerr(SSL_F_SSL_CTX_SET_SESSION_ID_CONTEXT,SSL_R_SSL_SESSION_ID_CONTEXT_TOO_LONG);
	return 0;
	}
    ctx->sid_ctx_length=sid_ctx_len;
    memcpy(ctx->sid_ctx,sid_ctx,sid_ctx_len);

    return 1;
    }

int SSL_set_session_id_context(SSL *ssl,const unsigned char *sid_ctx,
			       unsigned int sid_ctx_len)
    {
    if(sid_ctx_len > SSL_MAX_SID_CTX_LENGTH)
	{
	SSLerr(SSL_F_SSL_SET_SESSION_ID_CONTEXT,SSL_R_SSL_SESSION_ID_CONTEXT_TOO_LONG);
	return 0;
	}
    ssl->sid_ctx_length=sid_ctx_len;
    memcpy(ssl->sid_ctx,sid_ctx,sid_ctx_len);

    return 1;
    }

int SSL_CTX_set_generate_session_id(SSL_CTX *ctx, GEN_SESSION_CB cb)
	{
	CRYPTO_w_lock(CRYPTO_LOCK_SSL_CTX);
	ctx->generate_session_id = cb;
	CRYPTO_w_unlock(CRYPTO_LOCK_SSL_CTX);
	return 1;
	}

int SSL_set_generate_session_id(SSL *ssl, GEN_SESSION_CB cb)
	{
	CRYPTO_w_lock(CRYPTO_LOCK_SSL);
	ssl->generate_session_id = cb;
	CRYPTO_w_unlock(CRYPTO_LOCK_SSL);
	return 1;
	}

int SSL_has_matching_session_id(const SSL *ssl, const unsigned char *id,
				unsigned int id_len)
	{
	/* A quick examination of SSL_SESSION_hash and SSL_SESSION_cmp shows how
	 * we can "construct" a session to give us the desired check - ie. to
	 * find if there's a session in the hash table that would conflict with
	 * any new session built out of this id/id_len and the ssl_version in
	 * use by this SSL. */
	SSL_SESSION r, *p;

	if(id_len > sizeof r.session_id)
		return 0;

	r.ssl_version = ssl->version;
	r.session_id_length = id_len;
	memcpy(r.session_id, id, id_len);
	/* NB: SSLv2 always uses a fixed 16-byte session ID, so even if a
	 * callback is calling us to check the uniqueness of a shorter ID, it
	 * must be compared as a padded-out ID because that is what it will be
	 * converted to when the callback has finished choosing it. */
	if((r.ssl_version == SSL2_VERSION) &&
			(id_len < SSL2_SSL_SESSION_ID_LENGTH))
		{
		memset(r.session_id + id_len, 0,
			SSL2_SSL_SESSION_ID_LENGTH - id_len);
		r.session_id_length = SSL2_SSL_SESSION_ID_LENGTH;
		}

	CRYPTO_r_lock(CRYPTO_LOCK_SSL_CTX);
	p = (SSL_SESSION *)lh_retrieve(ssl->ctx->sessions, &r);
	CRYPTO_r_unlock(CRYPTO_LOCK_SSL_CTX);
	return (p != NULL);
	}

int SSL_CTX_set_purpose(SSL_CTX *s, int purpose)
	{
	return X509_VERIFY_PARAM_set_purpose(s->param, purpose);
	}

int SSL_set_purpose(SSL *s, int purpose)
	{
	return X509_VERIFY_PARAM_set_purpose(s->param, purpose);
	}

int SSL_CTX_set_trust(SSL_CTX *s, int trust)
	{
	return X509_VERIFY_PARAM_set_trust(s->param, trust);
	}

int SSL_set_trust(SSL *s, int trust)
	{
	return X509_VERIFY_PARAM_set_trust(s->param, trust);
	}

void SSL_free(SSL *s)
	{
	int i;

	if(s == NULL)
	    return;

	i=CRYPTO_add(&s->references,-1,CRYPTO_LOCK_SSL);
#ifdef REF_PRINT
	REF_PRINT("SSL",s);
#endif
	if (i > 0) return;
#ifdef REF_CHECK
	if (i < 0)
		{
		fprintf(stderr,"SSL_free, bad reference count\n");
		abort(); /* ok */
		}
#endif

	if (s->param)
		X509_VERIFY_PARAM_free(s->param);

	CRYPTO_free_ex_data(CRYPTO_EX_INDEX_SSL, s, &s->ex_data);

	if (s->bbio != NULL)
		{
		/* If the buffering BIO is in place, pop it off */
		if (s->bbio == s->wbio)
			{
			s->wbio=BIO_pop(s->wbio);
			}
		BIO_free(s->bbio);
		s->bbio=NULL;
		}
	if (s->rbio != NULL)
		BIO_free_all(s->rbio);
	if ((s->wbio != NULL) && (s->wbio != s->rbio))
		BIO_free_all(s->wbio);

	if (s->init_buf != NULL) BUF_MEM_free(s->init_buf);

	/* add extra stuff */
	if (s->cipher_list != NULL) sk_SSL_CIPHER_free(s->cipher_list);
	if (s->cipher_list_by_id != NULL) sk_SSL_CIPHER_free(s->cipher_list_by_id);

	/* Make the next call work :-) */
	if (s->session != NULL)
		{
		ssl_clear_bad_session(s);
		SSL_SESSION_free(s->session);
		}

	ssl_clear_cipher_ctx(s);

	if (s->cert != NULL) ssl_cert_free(s->cert);
	/* Free up if allocated */

#ifndef OPENSSL_NO_TLSEXT
	if (s->tlsext_hostname)
		OPENSSL_free(s->tlsext_hostname);
	if (s->initial_ctx) SSL_CTX_free(s->initial_ctx);
	if (s->tlsext_ocsp_exts)
		sk_X509_EXTENSION_pop_free(s->tlsext_ocsp_exts,
						X509_EXTENSION_free);
	if (s->tlsext_ocsp_ids)
		sk_OCSP_RESPID_pop_free(s->tlsext_ocsp_ids, OCSP_RESPID_free);
	if (s->tlsext_ocsp_resp)
		OPENSSL_free(s->tlsext_ocsp_resp);
#endif
	if (s->client_CA != NULL)
		sk_X509_NAME_pop_free(s->client_CA,X509_NAME_free);

	if (s->method != NULL) s->method->ssl_free(s);

	if (s->ctx) SSL_CTX_free(s->ctx);

#ifndef	OPENSSL_NO_KRB5
	if (s->kssl_ctx != NULL)
		kssl_ctx_free(s->kssl_ctx);
#endif	/* OPENSSL_NO_KRB5 */

	OPENSSL_free(s);
	}

void SSL_set_bio(SSL *s,BIO *rbio,BIO *wbio)
	{
	/* If the output buffering BIO is still in place, remove it
	 */
	if (s->bbio != NULL)
		{
		if (s->wbio == s->bbio)
			{
			s->wbio=s->wbio->next_bio;
			s->bbio->next_bio=NULL;
			}
		}
	if ((s->rbio != NULL) && (s->rbio != rbio))
		BIO_free_all(s->rbio);
	if ((s->wbio != NULL) && (s->wbio != wbio) && (s->rbio != s->wbio))
		BIO_free_all(s->wbio);
	s->rbio=rbio;
	s->wbio=wbio;
	}

BIO *SSL_get_rbio(const SSL *s)
	{ return(s->rbio); }

BIO *SSL_get_wbio(const SSL *s)
	{ return(s->wbio); }

int SSL_get_fd(const SSL *s)
	{
	return(SSL_get_rfd(s));
	}

int SSL_get_rfd(const SSL *s)
	{
	int ret= -1;
	BIO *b,*r;

	b=SSL_get_rbio(s);
	r=BIO_find_type(b,BIO_TYPE_DESCRIPTOR);
	if (r != NULL)
		BIO_get_fd(r,&ret);
	return(ret);
	}

int SSL_get_wfd(const SSL *s)
	{
	int ret= -1;
	BIO *b,*r;

	b=SSL_get_wbio(s);
	r=BIO_find_type(b,BIO_TYPE_DESCRIPTOR);
	if (r != NULL)
		BIO_get_fd(r,&ret);
	return(ret);
	}

#ifndef OPENSSL_NO_SOCK
int SSL_set_fd(SSL *s,int fd)
	{
	int ret=0;
	BIO *bio=NULL;

	bio=BIO_new(BIO_s_socket());

	if (bio == NULL)
		{
		SSLerr(SSL_F_SSL_SET_FD,ERR_R_BUF_LIB);
		goto err;
		}
	BIO_set_fd(bio,fd,BIO_NOCLOSE);
	SSL_set_bio(s,bio,bio);
	ret=1;
err:
	return(ret);
	}

int SSL_set_wfd(SSL *s,int fd)
	{
	int ret=0;
	BIO *bio=NULL;

	if ((s->rbio == NULL) || (BIO_method_type(s->rbio) != BIO_TYPE_SOCKET)
		|| ((int)BIO_get_fd(s->rbio,NULL) != fd))
		{
		bio=BIO_new(BIO_s_socket());

		if (bio == NULL)
			{ SSLerr(SSL_F_SSL_SET_WFD,ERR_R_BUF_LIB); goto err; }
		BIO_set_fd(bio,fd,BIO_NOCLOSE);
		SSL_set_bio(s,SSL_get_rbio(s),bio);
		}
	else
		SSL_set_bio(s,SSL_get_rbio(s),SSL_get_rbio(s));
	ret=1;
err:
	return(ret);
	}

int SSL_set_rfd(SSL *s,int fd)
	{
	int ret=0;
	BIO *bio=NULL;

	if ((s->wbio == NULL) || (BIO_method_type(s->wbio) != BIO_TYPE_SOCKET)
		|| ((int)BIO_get_fd(s->wbio,NULL) != fd))
		{
		bio=BIO_new(BIO_s_socket());

		if (bio == NULL)
			{
			SSLerr(SSL_F_SSL_SET_RFD,ERR_R_BUF_LIB);
			goto err;
			}
		BIO_set_fd(bio,fd,BIO_NOCLOSE);
		SSL_set_bio(s,bio,SSL_get_wbio(s));
		}
	else
		SSL_set_bio(s,SSL_get_wbio(s),SSL_get_wbio(s));
	ret=1;
err:
	return(ret);
	}
#endif


/* return length of latest Finished message we sent, copy to 'buf' */
size_t SSL_get_finished(const SSL *s, void *buf, size_t count)
	{
	size_t ret = 0;
	
	if (s->s3 != NULL)
		{
		ret = s->s3->tmp.finish_md_len;
		if (count > ret)
			count = ret;
		memcpy(buf, s->s3->tmp.finish_md, count);
		}
	return ret;
	}

/* return length of latest Finished message we expected, copy to 'buf' */
size_t SSL_get_peer_finished(const SSL *s, void *buf, size_t count)
	{
	size_t ret = 0;
	
	if (s->s3 != NULL)
		{
		ret = s->s3->tmp.peer_finish_md_len;
		if (count > ret)
			count = ret;
		memcpy(buf, s->s3->tmp.peer_finish_md, count);
		}
	return ret;
	}


int SSL_get_verify_mode(const SSL *s)
	{
	return(s->verify_mode);
	}

int SSL_get_verify_depth(const SSL *s)
	{
	return X509_VERIFY_PARAM_get_depth(s->param);
	}

int (*SSL_get_verify_callback(const SSL *s))(int,X509_STORE_CTX *)
	{
	return(s->verify_callback);
	}

int SSL_CTX_get_verify_mode(const SSL_CTX *ctx)
	{
	return(ctx->verify_mode);
	}

int SSL_CTX_get_verify_depth(const SSL_CTX *ctx)
	{
	return X509_VERIFY_PARAM_get_depth(ctx->param);
	}

int (*SSL_CTX_get_verify_callback(const SSL_CTX *ctx))(int,X509_STORE_CTX *)
	{
	return(ctx->default_verify_callback);
	}

void SSL_set_verify(SSL *s,int mode,
		    int (*callback)(int ok,X509_STORE_CTX *ctx))
	{
	s->verify_mode=mode;
	if (callback != NULL)
		s->verify_callback=callback;
	}

void SSL_set_verify_depth(SSL *s,int depth)
	{
	X509_VERIFY_PARAM_set_depth(s->param, depth);
	}

void SSL_set_read_ahead(SSL *s,int yes)
	{
	s->read_ahead=yes;
	}

int SSL_get_read_ahead(const SSL *s)
	{
	return(s->read_ahead);
	}

int SSL_pending(const SSL *s)
	{
	/* SSL_pending cannot work properly if read-ahead is enabled
	 * (SSL_[CTX_]ctrl(..., SSL_CTRL_SET_READ_AHEAD, 1, NULL)),
	 * and it is impossible to fix since SSL_pending cannot report
	 * errors that may be observed while scanning the new data.
	 * (Note that SSL_pending() is often used as a boolean value,
	 * so we'd better not return -1.)
	 */
	return(s->method->ssl_pending(s));
	}

X509 *SSL_get_peer_certificate(const SSL *s)
	{
	X509 *r;
	
	if ((s == NULL) || (s->session == NULL))
		r=NULL;
	else
		r=s->session->peer;

	if (r == NULL) return(r);

	CRYPTO_add(&r->references,1,CRYPTO_LOCK_X509);

	return(r);
	}

STACK_OF(X509) *SSL_get_peer_cert_chain(const SSL *s)
	{
	STACK_OF(X509) *r;
	
	if ((s == NULL) || (s->session == NULL) || (s->session->sess_cert == NULL))
		r=NULL;
	else
		r=s->session->sess_cert->cert_chain;

	/* If we are a client, cert_chain includes the peer's own
	 * certificate; if we are a server, it does not. */
	
	return(r);
	}

/* Now in theory, since the calling process own 't' it should be safe to
 * modify.  We need to be able to read f without being hassled */
void SSL_copy_session_id(SSL *t,const SSL *f)
	{
	CERT *tmp;

	/* Do we need to to SSL locking? */
	SSL_set_session(t,SSL_get_session(f));

	/* what if we are setup as SSLv2 but want to talk SSLv3 or
	 * vice-versa */
	if (t->method != f->method)
		{
		t->method->ssl_free(t);	/* cleanup current */
		t->method=f->method;	/* change method */
		t->method->ssl_new(t);	/* setup new */
		}

	tmp=t->cert;
	if (f->cert != NULL)
		{
		CRYPTO_add(&f->cert->references,1,CRYPTO_LOCK_SSL_CERT);
		t->cert=f->cert;
		}
	else
		t->cert=NULL;
	if (tmp != NULL) ssl_cert_free(tmp);
	SSL_set_session_id_context(t,f->sid_ctx,f->sid_ctx_length);
	}

/* Fix this so it checks all the valid key/cert options */
int SSL_CTX_check_private_key(const SSL_CTX *ctx)
	{
	if (	(ctx == NULL) ||
		(ctx->cert == NULL) ||
		(ctx->cert->key->x509 == NULL))
		{
		SSLerr(SSL_F_SSL_CTX_CHECK_PRIVATE_KEY,SSL_R_NO_CERTIFICATE_ASSIGNED);
		return(0);
		}
	if 	(ctx->cert->key->privatekey == NULL)
		{
		SSLerr(SSL_F_SSL_CTX_CHECK_PRIVATE_KEY,SSL_R_NO_PRIVATE_KEY_ASSIGNED);
		return(0);
		}
	return(X509_check_private_key(ctx->cert->key->x509, ctx->cert->key->privatekey));
	}

/* Fix this function so that it takes an optional type parameter */
int SSL_check_private_key(const SSL *ssl)
	{
	if (ssl == NULL)
		{
		SSLerr(SSL_F_SSL_CHECK_PRIVATE_KEY,ERR_R_PASSED_NULL_PARAMETER);
		return(0);
		}
	if (ssl->cert == NULL)
		{
                SSLerr(SSL_F_SSL_CHECK_PRIVATE_KEY,SSL_R_NO_CERTIFICATE_ASSIGNED);
		return 0;
		}
	if (ssl->cert->key->x509 == NULL)
		{
		SSLerr(SSL_F_SSL_CHECK_PRIVATE_KEY,SSL_R_NO_CERTIFICATE_ASSIGNED);
		return(0);
		}
	if (ssl->cert->key->privatekey == NULL)
		{
		SSLerr(SSL_F_SSL_CHECK_PRIVATE_KEY,SSL_R_NO_PRIVATE_KEY_ASSIGNED);
		return(0);
		}
	return(X509_check_private_key(ssl->cert->key->x509,
		ssl->cert->key->privatekey));
	}

int SSL_accept(SSL *s)
	{
	if (s->handshake_func == 0)
		/* Not properly initialized yet */
		SSL_set_accept_state(s);

	return(s->method->ssl_accept(s));
	}

int SSL_connect(SSL *s)
	{
	if (s->handshake_func == 0)
		/* Not properly initialized yet */
		SSL_set_connect_state(s);

	return(s->method->ssl_connect(s));
	}

long SSL_get_default_timeout(const SSL *s)
	{
	return(s->method->get_timeout());
	}

int SSL_read(SSL *s,void *buf,int num)
	{
	if (s->handshake_func == 0)
		{
		SSLerr(SSL_F_SSL_READ, SSL_R_UNINITIALIZED);
		return -1;
		}

	if (s->shutdown & SSL_RECEIVED_SHUTDOWN)
		{
		s->rwstate=SSL_NOTHING;
		return(0);
		}
	return(s->method->ssl_read(s,buf,num));
	}

int SSL_peek(SSL *s,void *buf,int num)
	{
	if (s->handshake_func == 0)
		{
		SSLerr(SSL_F_SSL_PEEK, SSL_R_UNINITIALIZED);
		return -1;
		}

	if (s->shutdown & SSL_RECEIVED_SHUTDOWN)
		{
		return(0);
		}
	return(s->method->ssl_peek(s,buf,num));
	}

int SSL_write(SSL *s,const void *buf,int num)
	{
	if (s->handshake_func == 0)
		{
		SSLerr(SSL_F_SSL_WRITE, SSL_R_UNINITIALIZED);
		return -1;
		}

	if (s->shutdown & SSL_SENT_SHUTDOWN)
		{
		s->rwstate=SSL_NOTHING;
		SSLerr(SSL_F_SSL_WRITE,SSL_R_PROTOCOL_IS_SHUTDOWN);
		return(-1);
		}
	return(s->method->ssl_write(s,buf,num));
	}

int SSL_shutdown(SSL *s)
	{
	/* Note that this function behaves differently from what one might
	 * expect.  Return values are 0 for no success (yet),
	 * 1 for success; but calling it once is usually not enough,
	 * even if blocking I/O is used (see ssl3_shutdown).
	 */

	if (s->handshake_func == 0)
		{
		SSLerr(SSL_F_SSL_SHUTDOWN, SSL_R_UNINITIALIZED);
		return -1;
		}

	if ((s != NULL) && !SSL_in_init(s))
		return(s->method->ssl_shutdown(s));
	else
		return(1);
	}

int SSL_renegotiate(SSL *s)
	{
	if (s->new_session == 0)
		{
		s->new_session=1;
		}
	return(s->method->ssl_renegotiate(s));
	}

int SSL_renegotiate_pending(SSL *s)
	{
	/* becomes true when negotiation is requested;
	 * false again once a handshake has finished */
	return (s->new_session != 0);
	}

long SSL_ctrl(SSL *s,int cmd,long larg,void *parg)
	{
	long l;

	switch (cmd)
		{
	case SSL_CTRL_GET_READ_AHEAD:
		return(s->read_ahead);
	case SSL_CTRL_SET_READ_AHEAD:
		l=s->read_ahead;
		s->read_ahead=larg;
		return(l);

	case SSL_CTRL_SET_MSG_CALLBACK_ARG:
		s->msg_callback_arg = parg;
		return 1;

	case SSL_CTRL_OPTIONS:
		return(s->options|=larg);
	case SSL_CTRL_CLEAR_OPTIONS:
		return(s->options&=~larg);
	case SSL_CTRL_MODE:
		return(s->mode|=larg);
	case SSL_CTRL_CLEAR_MODE:
		return(s->mode &=~larg);
	case SSL_CTRL_GET_MAX_CERT_LIST:
		return(s->max_cert_list);
	case SSL_CTRL_SET_MAX_CERT_LIST:
		l=s->max_cert_list;
		s->max_cert_list=larg;
		return(l);
	case SSL_CTRL_SET_MTU:
#ifndef OPENSSL_NO_DTLS1
		if (larg < (long)dtls1_min_mtu())
			return 0;
#endif

		if (SSL_version(s) == DTLS1_VERSION ||
		    SSL_version(s) == DTLS1_BAD_VER)
			{
			s->d1->mtu = larg;
			return larg;
			}
		return 0;
	case SSL_CTRL_GET_RI_SUPPORT:
		if (s->s3)
			return s->s3->send_connection_binding;
		else return 0;
	default:
		return(s->method->ssl_ctrl(s,cmd,larg,parg));
		}
	}

long SSL_callback_ctrl(SSL *s, int cmd, void (*fp)(void))
	{
	switch(cmd)
		{
	case SSL_CTRL_SET_MSG_CALLBACK:
		s->msg_callback = (void (*)(int write_p, int version, int content_type, const void *buf, size_t len, SSL *ssl, void *arg))(fp);
		return 1;
		
	default:
		return(s->method->ssl_callback_ctrl(s,cmd,fp));
		}
	}

struct lhash_st *SSL_CTX_sessions(SSL_CTX *ctx)
	{
	return ctx->sessions;
	}

long SSL_CTX_ctrl(SSL_CTX *ctx,int cmd,long larg,void *parg)
	{
	long l;

	switch (cmd)
		{
	case SSL_CTRL_GET_READ_AHEAD:
		return(ctx->read_ahead);
	case SSL_CTRL_SET_READ_AHEAD:
		l=ctx->read_ahead;
		ctx->read_ahead=larg;
		return(l);
		
	case SSL_CTRL_SET_MSG_CALLBACK_ARG:
		ctx->msg_callback_arg = parg;
		return 1;

	case SSL_CTRL_GET_MAX_CERT_LIST:
		return(ctx->max_cert_list);
	case SSL_CTRL_SET_MAX_CERT_LIST:
		l=ctx->max_cert_list;
		ctx->max_cert_list=larg;
		return(l);

	case SSL_CTRL_SET_SESS_CACHE_SIZE:
		l=ctx->session_cache_size;
		ctx->session_cache_size=larg;
		return(l);
	case SSL_CTRL_GET_SESS_CACHE_SIZE:
		return(ctx->session_cache_size);
	case SSL_CTRL_SET_SESS_CACHE_MODE:
		l=ctx->session_cache_mode;
		ctx->session_cache_mode=larg;
		return(l);
	case SSL_CTRL_GET_SESS_CACHE_MODE:
		return(ctx->session_cache_mode);

	case SSL_CTRL_SESS_NUMBER:
		return(ctx->sessions->num_items);
	case SSL_CTRL_SESS_CONNECT:
		return(ctx->stats.sess_connect);
	case SSL_CTRL_SESS_CONNECT_GOOD:
		return(ctx->stats.sess_connect_good);
	case SSL_CTRL_SESS_CONNECT_RENEGOTIATE:
		return(ctx->stats.sess_connect_renegotiate);
	case SSL_CTRL_SESS_ACCEPT:
		return(ctx->stats.sess_accept);
	case SSL_CTRL_SESS_ACCEPT_GOOD:
		return(ctx->stats.sess_accept_good);
	case SSL_CTRL_SESS_ACCEPT_RENEGOTIATE:
		return(ctx->stats.sess_accept_renegotiate);
	case SSL_CTRL_SESS_HIT:
		return(ctx->stats.sess_hit);
	case SSL_CTRL_SESS_CB_HIT:
		return(ctx->stats.sess_cb_hit);
	case SSL_CTRL_SESS_MISSES:
		return(ctx->stats.sess_miss);
	case SSL_CTRL_SESS_TIMEOUTS:
		return(ctx->stats.sess_timeout);
	case SSL_CTRL_SESS_CACHE_FULL:
		return(ctx->stats.sess_cache_full);
	case SSL_CTRL_OPTIONS:
		return(ctx->options|=larg);
	case SSL_CTRL_CLEAR_OPTIONS:
		return(ctx->options&=~larg);
	case SSL_CTRL_MODE:
		return(ctx->mode|=larg);
	case SSL_CTRL_CLEAR_MODE:
		return(ctx->mode&=~larg);
	default:
		return(ctx->method->ssl_ctx_ctrl(ctx,cmd,larg,parg));
		}
	}

long SSL_CTX_callback_ctrl(SSL_CTX *ctx, int cmd, void (*fp)(void))
	{
	switch(cmd)
		{
	case SSL_CTRL_SET_MSG_CALLBACK:
		ctx->msg_callback = (void (*)(int write_p, int version, int content_type, const void *buf, size_t len, SSL *ssl, void *arg))(fp);
		return 1;

	default:
		return(ctx->method->ssl_ctx_callback_ctrl(ctx,cmd,fp));
		}
	}

int ssl_cipher_id_cmp(const SSL_CIPHER *a, const SSL_CIPHER *b)
	{
	long l;

	l=a->id-b->id;
	if (l == 0L)
		return(0);
	else
		return((l > 0)?1:-1);
	}

int ssl_cipher_ptr_id_cmp(const SSL_CIPHER * const *ap,
			const SSL_CIPHER * const *bp)
	{
	long l;

	l=(*ap)->id-(*bp)->id;
	if (l == 0L)
		return(0);
	else
		return((l > 0)?1:-1);
	}

/** return a STACK of the ciphers available for the SSL and in order of
 * preference */
STACK_OF(SSL_CIPHER) *SSL_get_ciphers(const SSL *s)
	{
	if (s != NULL)
		{
		if (s->cipher_list != NULL)
			{
			return(s->cipher_list);
			}
		else if ((s->ctx != NULL) &&
			(s->ctx->cipher_list != NULL))
			{
			return(s->ctx->cipher_list);
			}
		}
	return(NULL);
	}

/** return a STACK of the ciphers available for the SSL and in order of
 * algorithm id */
STACK_OF(SSL_CIPHER) *ssl_get_ciphers_by_id(SSL *s)
	{
	if (s != NULL)
		{
		if (s->cipher_list_by_id != NULL)
			{
			return(s->cipher_list_by_id);
			}
		else if ((s->ctx != NULL) &&
			(s->ctx->cipher_list_by_id != NULL))
			{
			return(s->ctx->cipher_list_by_id);
			}
		}
	return(NULL);
	}

/** The old interface to get the same thing as SSL_get_ciphers() */
const char *SSL_get_cipher_list(const SSL *s,int n)
	{
	SSL_CIPHER *c;
	STACK_OF(SSL_CIPHER) *sk;

	if (s == NULL) return(NULL);
	sk=SSL_get_ciphers(s);
	if ((sk == NULL) || (sk_SSL_CIPHER_num(sk) <= n))
		return(NULL);
	c=sk_SSL_CIPHER_value(sk,n);
	if (c == NULL) return(NULL);
	return(c->name);
	}

/** specify the ciphers to be used by default by the SSL_CTX */
int SSL_CTX_set_cipher_list(SSL_CTX *ctx, const char *str)
	{
	STACK_OF(SSL_CIPHER) *sk;
	
	sk=ssl_create_cipher_list(ctx->method,&ctx->cipher_list,
		&ctx->cipher_list_by_id,str);
	/* ssl_create_cipher_list may return an empty stack if it
	 * was unable to find a cipher matching the given rule string
	 * (for example if the rule string specifies a cipher which
	 * has been disabled). This is not an error as far as 
	 * ssl_create_cipher_list is concerned, and hence 
	 * ctx->cipher_list and ctx->cipher_list_by_id has been
	 * updated. */
	if (sk == NULL)
		return 0;
	else if (sk_SSL_CIPHER_num(sk) == 0)
		{
		SSLerr(SSL_F_SSL_CTX_SET_CIPHER_LIST, SSL_R_NO_CIPHER_MATCH);
		return 0;
		}
	return 1;
	}

/** specify the ciphers to be used by the SSL */
int SSL_set_cipher_list(SSL *s,const char *str)
	{
	STACK_OF(SSL_CIPHER) *sk;
	
	sk=ssl_create_cipher_list(s->ctx->method,&s->cipher_list,
		&s->cipher_list_by_id,str);
	/* see comment in SSL_CTX_set_cipher_list */
	if (sk == NULL)
		return 0;
	else if (sk_SSL_CIPHER_num(sk) == 0)
		{
		SSLerr(SSL_F_SSL_SET_CIPHER_LIST, SSL_R_NO_CIPHER_MATCH);
		return 0;
		}
	return 1;
	}

/* works well for SSLv2, not so good for SSLv3 */
char *SSL_get_shared_ciphers(const SSL *s,char *buf,int len)
	{
	char *p;
	STACK_OF(SSL_CIPHER) *sk;
	SSL_CIPHER *c;
	int i;

	if ((s->session == NULL) || (s->session->ciphers == NULL) ||
		(len < 2))
		return(NULL);

	p=buf;
	sk=s->session->ciphers;

	if (sk_SSL_CIPHER_num(sk) == 0)
		return NULL;

	for (i=0; i<sk_SSL_CIPHER_num(sk); i++)
		{
		int n;

		c=sk_SSL_CIPHER_value(sk,i);
		n=strlen(c->name);
		if (n+1 > len)
			{
			if (p != buf)
				--p;
			*p='\0';
			return buf;
			}
		strcpy(p,c->name);
		p+=n;
		*(p++)=':';
		len-=n+1;
		}
	p[-1]='\0';
	return(buf);
	}

int ssl_cipher_list_to_bytes(SSL *s,STACK_OF(SSL_CIPHER) *sk,unsigned char *p,
                             int (*put_cb)(const SSL_CIPHER *, unsigned char *))
	{
	int i,j=0;
	SSL_CIPHER *c;
	unsigned char *q;
#ifndef OPENSSL_NO_KRB5
        int nokrb5 = !kssl_tgt_is_available(s->kssl_ctx);
#endif /* OPENSSL_NO_KRB5 */

	if (sk == NULL) return(0);
	q=p;
	if (put_cb == NULL)
		put_cb = s->method->put_cipher_by_char;

	for (i=0; i<sk_SSL_CIPHER_num(sk); i++)
		{
		c=sk_SSL_CIPHER_value(sk,i);
#ifndef OPENSSL_NO_KRB5
                if ((c->algorithms & SSL_KRB5) && nokrb5)
                    continue;
#endif /* OPENSSL_NO_KRB5 */                    

		j = put_cb(c,p);
		p+=j;
		}
	/* If p == q, no ciphers; caller indicates an error.
	 * Otherwise, add applicable SCSVs. */
	if (p != q)
		{
		if (!s->new_session)
			{
			static SSL_CIPHER scsv =
				{
				0, NULL, SSL3_CK_SCSV, 0, 0, 0, 0, 0, 0, 0,
				};
			j = put_cb(&scsv,p);
			p+=j;
#ifdef OPENSSL_RI_DEBUG
			fprintf(stderr, "TLS_EMPTY_RENEGOTIATION_INFO_SCSV sent by client\n");
#endif
			}

		if (s->mode & SSL_MODE_SEND_FALLBACK_SCSV)
			{
			static SSL_CIPHER scsv =
				{
				0, NULL, SSL3_CK_FALLBACK_SCSV, 0, 0, 0, 0, 0, 0, 0,
				};
			j = put_cb(&scsv,p);
			p+=j;
			}
 		}

	return(p-q);
	}

STACK_OF(SSL_CIPHER) *ssl_bytes_to_cipher_list(SSL *s,unsigned char *p,int num,
					       STACK_OF(SSL_CIPHER) **skp)
	{
	SSL_CIPHER *c;
	STACK_OF(SSL_CIPHER) *sk;
	int i,n;

	if (s->s3)
		s->s3->send_connection_binding = 0;

	n=ssl_put_cipher_by_char(s,NULL,NULL);
	if (n == 0 || (num%n) != 0)
		{
		SSLerr(SSL_F_SSL_BYTES_TO_CIPHER_LIST,SSL_R_ERROR_IN_RECEIVED_CIPHER_LIST);
		return(NULL);
		}
	if ((skp == NULL) || (*skp == NULL))
		sk=sk_SSL_CIPHER_new_null(); /* change perhaps later */
	else
		{
		sk= *skp;
		sk_SSL_CIPHER_zero(sk);
		}

	for (i=0; i<num; i+=n)
		{
		/* Check for TLS_EMPTY_RENEGOTIATION_INFO_SCSV */
		if (s->s3 && (n != 3 || !p[0]) &&
			(p[n-2] == ((SSL3_CK_SCSV >> 8) & 0xff)) &&
			(p[n-1] == (SSL3_CK_SCSV & 0xff)))
			{
			/* SCSV fatal if renegotiating */
			if (s->new_session)
				{
				SSLerr(SSL_F_SSL_BYTES_TO_CIPHER_LIST,SSL_R_SCSV_RECEIVED_WHEN_RENEGOTIATING);
				ssl3_send_alert(s,SSL3_AL_FATAL,SSL_AD_HANDSHAKE_FAILURE); 
				goto err;
				}
			s->s3->send_connection_binding = 1;
			p += n;
#ifdef OPENSSL_RI_DEBUG
			fprintf(stderr, "SCSV received by server\n");
#endif
			continue;
			}

		/* Check for TLS_FALLBACK_SCSV */
		if ((n != 3 || !p[0]) &&
			(p[n-2] == ((SSL3_CK_FALLBACK_SCSV >> 8) & 0xff)) &&
			(p[n-1] == (SSL3_CK_FALLBACK_SCSV & 0xff)))
			{
			/* The SCSV indicates that the client previously tried a higher version.
			 * Fail if the current version is an unexpected downgrade. */
			if (!SSL_ctrl(s, SSL_CTRL_CHECK_PROTO_VERSION, 0, NULL))
				{
				SSLerr(SSL_F_SSL_BYTES_TO_CIPHER_LIST,SSL_R_INAPPROPRIATE_FALLBACK);
				if (s->s3)
					ssl3_send_alert(s,SSL3_AL_FATAL,SSL_AD_INAPPROPRIATE_FALLBACK);
				goto err;
				}
			continue;
			}

		c=ssl_get_cipher_by_char(s,p);
		p+=n;
		if (c != NULL)
			{
			if (!sk_SSL_CIPHER_push(sk,c))
				{
				SSLerr(SSL_F_SSL_BYTES_TO_CIPHER_LIST,ERR_R_MALLOC_FAILURE);
				goto err;
				}
			}
		}

	if (skp != NULL)
		*skp=sk;
	return(sk);
err:
	if ((skp == NULL) || (*skp == NULL))
		sk_SSL_CIPHER_free(sk);
	return(NULL);
	}

#ifndef OPENSSL_NO_TLSEXT
/** return a servername extension value if provided in Client Hello, or NULL.
 * So far, only host_name types are defined (RFC 3546).
 */

const char *SSL_get_servername(const SSL *s, const int type)
	{
	if (type != TLSEXT_NAMETYPE_host_name)
		return NULL;

	return s->session && !s->tlsext_hostname ?
		s->session->tlsext_hostname :
		s->tlsext_hostname;
	}

int SSL_get_servername_type(const SSL *s)
	{
	if (s->session && (!s->tlsext_hostname ? s->session->tlsext_hostname : s->tlsext_hostname))
		return TLSEXT_NAMETYPE_host_name;
	return -1;
	}
#endif

unsigned long SSL_SESSION_hash(const SSL_SESSION *a)
	{
	unsigned long l;

	l=(unsigned long)
		((unsigned int) a->session_id[0]     )|
		((unsigned int) a->session_id[1]<< 8L)|
		((unsigned long)a->session_id[2]<<16L)|
		((unsigned long)a->session_id[3]<<24L);
	return(l);
	}

/* NB: If this function (or indeed the hash function which uses a sort of
 * coarser function than this one) is changed, ensure
 * SSL_CTX_has_matching_session_id() is checked accordingly. It relies on being
 * able to construct an SSL_SESSION that will collide with any existing session
 * with a matching session ID. */
int SSL_SESSION_cmp(const SSL_SESSION *a,const SSL_SESSION *b)
	{
	if (a->ssl_version != b->ssl_version)
		return(1);
	if (a->session_id_length != b->session_id_length)
		return(1);
	return(memcmp(a->session_id,b->session_id,a->session_id_length));
	}

/* These wrapper functions should remain rather than redeclaring
 * SSL_SESSION_hash and SSL_SESSION_cmp for void* types and casting each
 * variable. The reason is that the functions aren't static, they're exposed via
 * ssl.h. */
static IMPLEMENT_LHASH_HASH_FN(SSL_SESSION_hash, SSL_SESSION *)
static IMPLEMENT_LHASH_COMP_FN(SSL_SESSION_cmp, SSL_SESSION *)

SSL_CTX *SSL_CTX_new(SSL_METHOD *meth)
	{
	SSL_CTX *ret=NULL;
	
	if (meth == NULL)
		{
		SSLerr(SSL_F_SSL_CTX_NEW,SSL_R_NULL_SSL_METHOD_PASSED);
		return(NULL);
		}

#ifdef OPENSSL_FIPS
	if (FIPS_mode() && (meth->version < TLS1_VERSION))	
		{
		SSLerr(SSL_F_SSL_CTX_NEW, SSL_R_ONLY_TLS_ALLOWED_IN_FIPS_MODE);
		return NULL;
		}
#endif

	if (SSL_get_ex_data_X509_STORE_CTX_idx() < 0)
		{
		SSLerr(SSL_F_SSL_CTX_NEW,SSL_R_X509_VERIFICATION_SETUP_PROBLEMS);
		goto err;
		}
	ret=(SSL_CTX *)OPENSSL_malloc(sizeof(SSL_CTX));
	if (ret == NULL)
		goto err;

	memset(ret,0,sizeof(SSL_CTX));

	ret->method=meth;

	ret->cert_store=NULL;
	ret->session_cache_mode=SSL_SESS_CACHE_SERVER;
	ret->session_cache_size=SSL_SESSION_CACHE_MAX_SIZE_DEFAULT;
	ret->session_cache_head=NULL;
	ret->session_cache_tail=NULL;

	/* We take the system default */
	ret->session_timeout=meth->get_timeout();

	ret->new_session_cb=0;
	ret->remove_session_cb=0;
	ret->get_session_cb=0;
	ret->generate_session_id=0;

	memset((char *)&ret->stats,0,sizeof(ret->stats));

	ret->references=1;
	ret->quiet_shutdown=0;

/*	ret->cipher=NULL;*/
/*	ret->s2->challenge=NULL;
	ret->master_key=NULL;
	ret->key_arg=NULL;
	ret->s2->conn_id=NULL; */

	ret->info_callback=NULL;

	ret->app_verify_callback=0;
	ret->app_verify_arg=NULL;

	ret->max_cert_list=SSL_MAX_CERT_LIST_DEFAULT;
	ret->read_ahead=0;
	ret->msg_callback=0;
	ret->msg_callback_arg=NULL;
	ret->verify_mode=SSL_VERIFY_NONE;
#if 0
	ret->verify_depth=-1; /* Don't impose a limit (but x509_lu.c does) */
#endif
	ret->sid_ctx_length=0;
	ret->default_verify_callback=NULL;
	if ((ret->cert=ssl_cert_new()) == NULL)
		goto err;

	ret->default_passwd_callback=0;
	ret->default_passwd_callback_userdata=NULL;
	ret->client_cert_cb=0;
	ret->app_gen_cookie_cb=0;
	ret->app_verify_cookie_cb=0;

	ret->sessions=lh_new(LHASH_HASH_FN(SSL_SESSION_hash),
			LHASH_COMP_FN(SSL_SESSION_cmp));
	if (ret->sessions == NULL) goto err;
	ret->cert_store=X509_STORE_new();
	if (ret->cert_store == NULL) goto err;

	ssl_create_cipher_list(ret->method,
		&ret->cipher_list,&ret->cipher_list_by_id,
		SSL_DEFAULT_CIPHER_LIST);
	if (ret->cipher_list == NULL
	    || sk_SSL_CIPHER_num(ret->cipher_list) <= 0)
		{
		SSLerr(SSL_F_SSL_CTX_NEW,SSL_R_LIBRARY_HAS_NO_CIPHERS);
		goto err2;
		}

	ret->param = X509_VERIFY_PARAM_new();
	if (!ret->param)
		goto err;

	if ((ret->rsa_md5=EVP_get_digestbyname("ssl2-md5")) == NULL)
		{
		SSLerr(SSL_F_SSL_CTX_NEW,SSL_R_UNABLE_TO_LOAD_SSL2_MD5_ROUTINES);
		goto err2;
		}
	if ((ret->md5=EVP_get_digestbyname("ssl3-md5")) == NULL)
		{
		SSLerr(SSL_F_SSL_CTX_NEW,SSL_R_UNABLE_TO_LOAD_SSL3_MD5_ROUTINES);
		goto err2;
		}
	if ((ret->sha1=EVP_get_digestbyname("ssl3-sha1")) == NULL)
		{
		SSLerr(SSL_F_SSL_CTX_NEW,SSL_R_UNABLE_TO_LOAD_SSL3_SHA1_ROUTINES);
		goto err2;
		}

	if ((ret->client_CA=sk_X509_NAME_new_null()) == NULL)
		goto err;

	CRYPTO_new_ex_data(CRYPTO_EX_INDEX_SSL_CTX, ret, &ret->ex_data);

	ret->extra_certs=NULL;
	/* No compression for DTLS */
	if (meth->version != DTLS1_VERSION)
		ret->comp_methods=SSL_COMP_get_compression_methods();

#ifndef OPENSSL_NO_TLSEXT
	ret->tlsext_servername_callback = 0;
	ret->tlsext_servername_arg = NULL;
	/* Setup RFC4507 ticket keys */
	if ((RAND_pseudo_bytes(ret->tlsext_tick_key_name, 16) <= 0)
		|| (RAND_bytes(ret->tlsext_tick_hmac_key, 16) <= 0)
		|| (RAND_bytes(ret->tlsext_tick_aes_key, 16) <= 0))
		ret->options |= SSL_OP_NO_TICKET;

	ret->tlsext_status_cb = 0;
	ret->tlsext_status_arg = NULL;

#endif

#ifndef OPENSSL_NO_ENGINE
	ret->client_cert_engine = NULL;
#ifdef OPENSSL_SSL_CLIENT_ENGINE_AUTO
#define eng_strx(x)	#x
#define eng_str(x)	eng_strx(x)
	/* Use specific client engine automatically... ignore errors */
	{
	ENGINE *eng;
	eng = ENGINE_by_id(eng_str(OPENSSL_SSL_CLIENT_ENGINE_AUTO));
	if (!eng)
		{
		ERR_clear_error();
		ENGINE_load_builtin_engines();
		eng = ENGINE_by_id(eng_str(OPENSSL_SSL_CLIENT_ENGINE_AUTO));
		}
	if (!eng || !SSL_CTX_set_client_cert_engine(ret, eng))
		ERR_clear_error();
	}
#endif
#endif
	/* Default is to connect to non-RI servers. When RI is more widely
	 * deployed might change this.
	 */
	ret->options |= SSL_OP_LEGACY_SERVER_CONNECT;

	return(ret);
err:
	SSLerr(SSL_F_SSL_CTX_NEW,ERR_R_MALLOC_FAILURE);
err2:
	if (ret != NULL) SSL_CTX_free(ret);
	return(NULL);
	}

#if 0
static void SSL_COMP_free(SSL_COMP *comp)
    { OPENSSL_free(comp); }
#endif

void SSL_CTX_free(SSL_CTX *a)
	{
	int i;

	if (a == NULL) return;

	i=CRYPTO_add(&a->references,-1,CRYPTO_LOCK_SSL_CTX);
#ifdef REF_PRINT
	REF_PRINT("SSL_CTX",a);
#endif
	if (i > 0) return;
#ifdef REF_CHECK
	if (i < 0)
		{
		fprintf(stderr,"SSL_CTX_free, bad reference count\n");
		abort(); /* ok */
		}
#endif

	if (a->param)
		X509_VERIFY_PARAM_free(a->param);

	/*
	 * Free internal session cache. However: the remove_cb() may reference
	 * the ex_data of SSL_CTX, thus the ex_data store can only be removed
	 * after the sessions were flushed.
	 * As the ex_data handling routines might also touch the session cache,
	 * the most secure solution seems to be: empty (flush) the cache, then
	 * free ex_data, then finally free the cache.
	 * (See ticket [openssl.org #212].)
	 */
	if (a->sessions != NULL)
		SSL_CTX_flush_sessions(a,0);

	CRYPTO_free_ex_data(CRYPTO_EX_INDEX_SSL_CTX, a, &a->ex_data);

	if (a->sessions != NULL)
		lh_free(a->sessions);

	if (a->cert_store != NULL)
		X509_STORE_free(a->cert_store);
	if (a->cipher_list != NULL)
		sk_SSL_CIPHER_free(a->cipher_list);
	if (a->cipher_list_by_id != NULL)
		sk_SSL_CIPHER_free(a->cipher_list_by_id);
	if (a->cert != NULL)
		ssl_cert_free(a->cert);
	if (a->client_CA != NULL)
		sk_X509_NAME_pop_free(a->client_CA,X509_NAME_free);
	if (a->extra_certs != NULL)
		sk_X509_pop_free(a->extra_certs,X509_free);
#if 0 /* This should never be done, since it removes a global database */
	if (a->comp_methods != NULL)
		sk_SSL_COMP_pop_free(a->comp_methods,SSL_COMP_free);
#else
	a->comp_methods = NULL;
#endif
#ifndef OPENSSL_NO_ENGINE
	if (a->client_cert_engine)
		ENGINE_finish(a->client_cert_engine);
#endif
	OPENSSL_free(a);
	}

void SSL_CTX_set_default_passwd_cb(SSL_CTX *ctx, pem_password_cb *cb)
	{
	ctx->default_passwd_callback=cb;
	}

void SSL_CTX_set_default_passwd_cb_userdata(SSL_CTX *ctx,void *u)
	{
	ctx->default_passwd_callback_userdata=u;
	}

void SSL_CTX_set_cert_verify_callback(SSL_CTX *ctx, int (*cb)(X509_STORE_CTX *,void *), void *arg)
	{
	ctx->app_verify_callback=cb;
	ctx->app_verify_arg=arg;
	}

void SSL_CTX_set_verify(SSL_CTX *ctx,int mode,int (*cb)(int, X509_STORE_CTX *))
	{
	ctx->verify_mode=mode;
	ctx->default_verify_callback=cb;
	}

void SSL_CTX_set_verify_depth(SSL_CTX *ctx,int depth)
	{
	X509_VERIFY_PARAM_set_depth(ctx->param, depth);
	}

void ssl_set_cert_masks(CERT *c, SSL_CIPHER *cipher)
	{
	CERT_PKEY *cpk;
	int rsa_enc,rsa_tmp,rsa_sign,dh_tmp,dh_rsa,dh_dsa,dsa_sign;
	int rsa_enc_export,dh_rsa_export,dh_dsa_export;
	int rsa_tmp_export,dh_tmp_export,kl;
	unsigned long mask,emask;
	int have_ecc_cert, ecdh_ok, ecdsa_ok, ecc_pkey_size;
#ifndef OPENSSL_NO_ECDH
	int have_ecdh_tmp;
#endif
	X509 *x = NULL;
	EVP_PKEY *ecc_pkey = NULL;
	int signature_nid = 0;

	if (c == NULL) return;

	kl=SSL_C_EXPORT_PKEYLENGTH(cipher);

#ifndef OPENSSL_NO_RSA
	rsa_tmp=(c->rsa_tmp != NULL || c->rsa_tmp_cb != NULL);
	rsa_tmp_export=(c->rsa_tmp_cb != NULL ||
		(rsa_tmp && RSA_size(c->rsa_tmp)*8 <= kl));
#else
	rsa_tmp=rsa_tmp_export=0;
#endif
#ifndef OPENSSL_NO_DH
	dh_tmp=(c->dh_tmp != NULL || c->dh_tmp_cb != NULL);
	dh_tmp_export=(c->dh_tmp_cb != NULL ||
		(dh_tmp && DH_size(c->dh_tmp)*8 <= kl));
#else
	dh_tmp=dh_tmp_export=0;
#endif

#ifndef OPENSSL_NO_ECDH
	have_ecdh_tmp=(c->ecdh_tmp != NULL || c->ecdh_tmp_cb != NULL);
#endif
	cpk= &(c->pkeys[SSL_PKEY_RSA_ENC]);
	rsa_enc= (cpk->x509 != NULL && cpk->privatekey != NULL);
	rsa_enc_export=(rsa_enc && EVP_PKEY_size(cpk->privatekey)*8 <= kl);
	cpk= &(c->pkeys[SSL_PKEY_RSA_SIGN]);
	rsa_sign=(cpk->x509 != NULL && cpk->privatekey != NULL);
	cpk= &(c->pkeys[SSL_PKEY_DSA_SIGN]);
	dsa_sign=(cpk->x509 != NULL && cpk->privatekey != NULL);
	cpk= &(c->pkeys[SSL_PKEY_DH_RSA]);
	dh_rsa=  (cpk->x509 != NULL && cpk->privatekey != NULL);
	dh_rsa_export=(dh_rsa && EVP_PKEY_size(cpk->privatekey)*8 <= kl);
	cpk= &(c->pkeys[SSL_PKEY_DH_DSA]);
/* FIX THIS EAY EAY EAY */
	dh_dsa=  (cpk->x509 != NULL && cpk->privatekey != NULL);
	dh_dsa_export=(dh_dsa && EVP_PKEY_size(cpk->privatekey)*8 <= kl);
	cpk= &(c->pkeys[SSL_PKEY_ECC]);
	have_ecc_cert= (cpk->x509 != NULL && cpk->privatekey != NULL);
	mask=0;
	emask=0;

#ifdef CIPHER_DEBUG
	printf("rt=%d rte=%d dht=%d re=%d ree=%d rs=%d ds=%d dhr=%d dhd=%d\n",
		rsa_tmp,rsa_tmp_export,dh_tmp,
		rsa_enc,rsa_enc_export,rsa_sign,dsa_sign,dh_rsa,dh_dsa);
#endif

	if (rsa_enc || (rsa_tmp && rsa_sign))
		mask|=SSL_kRSA;
	if (rsa_enc_export || (rsa_tmp_export && (rsa_sign || rsa_enc)))
		emask|=SSL_kRSA;

#if 0
	/* The match needs to be both kEDH and aRSA or aDSA, so don't worry */
	if (	(dh_tmp || dh_rsa || dh_dsa) && 
		(rsa_enc || rsa_sign || dsa_sign))
		mask|=SSL_kEDH;
	if ((dh_tmp_export || dh_rsa_export || dh_dsa_export) &&
		(rsa_enc || rsa_sign || dsa_sign))
		emask|=SSL_kEDH;
#endif

	if (dh_tmp_export) 
		emask|=SSL_kEDH;

	if (dh_tmp)
		mask|=SSL_kEDH;

	if (dh_rsa) mask|=SSL_kDHr;
	if (dh_rsa_export) emask|=SSL_kDHr;

	if (dh_dsa) mask|=SSL_kDHd;
	if (dh_dsa_export) emask|=SSL_kDHd;

	if (rsa_enc || rsa_sign)
		{
		mask|=SSL_aRSA;
		emask|=SSL_aRSA;
		}

	if (dsa_sign)
		{
		mask|=SSL_aDSS;
		emask|=SSL_aDSS;
		}

	mask|=SSL_aNULL;
	emask|=SSL_aNULL;

#ifndef OPENSSL_NO_KRB5
	mask|=SSL_kKRB5|SSL_aKRB5;
	emask|=SSL_kKRB5|SSL_aKRB5;
#endif

	/* An ECC certificate may be usable for ECDH and/or
	 * ECDSA cipher suites depending on the key usage extension.
	 */
	if (have_ecc_cert)
		{
                /* This call populates extension flags (ex_flags) */
		x = (c->pkeys[SSL_PKEY_ECC]).x509;
		X509_check_purpose(x, -1, 0);
		ecdh_ok = (x->ex_flags & EXFLAG_KUSAGE) ?
		    (x->ex_kusage & X509v3_KU_KEY_AGREEMENT) : 1;
		ecdsa_ok = (x->ex_flags & EXFLAG_KUSAGE) ?
		    (x->ex_kusage & X509v3_KU_DIGITAL_SIGNATURE) : 1;
		ecc_pkey = X509_get_pubkey(x);
		ecc_pkey_size = (ecc_pkey != NULL) ? 
		    EVP_PKEY_bits(ecc_pkey) : 0;
		EVP_PKEY_free(ecc_pkey);
		if ((x->sig_alg) && (x->sig_alg->algorithm))
			signature_nid = OBJ_obj2nid(x->sig_alg->algorithm);
#ifndef OPENSSL_NO_ECDH
		if (ecdh_ok)
			{
			if ((signature_nid == NID_md5WithRSAEncryption) ||
			    (signature_nid == NID_md4WithRSAEncryption) ||
			    (signature_nid == NID_md2WithRSAEncryption))
				{
				mask|=SSL_kECDH|SSL_aRSA;
				if (ecc_pkey_size <= 163)
					emask|=SSL_kECDH|SSL_aRSA;
				}
			if (signature_nid == NID_ecdsa_with_SHA1)
				{
				mask|=SSL_kECDH|SSL_aECDSA;
				if (ecc_pkey_size <= 163)
					emask|=SSL_kECDH|SSL_aECDSA;
				}
			}
#endif
#ifndef OPENSSL_NO_ECDSA
		if (ecdsa_ok)
			{
			mask|=SSL_aECDSA;
			emask|=SSL_aECDSA;
			}
#endif
		}

#ifndef OPENSSL_NO_ECDH
	if (have_ecdh_tmp)
		{
		mask|=SSL_kECDHE;
		emask|=SSL_kECDHE;
		}
#endif
	c->mask=mask;
	c->export_mask=emask;
	c->valid=1;
	}

/* This handy macro borrowed from crypto/x509v3/v3_purp.c */
#define ku_reject(x, usage) \
	(((x)->ex_flags & EXFLAG_KUSAGE) && !((x)->ex_kusage & (usage)))

int check_srvr_ecc_cert_and_alg(X509 *x, SSL_CIPHER *cs)
	{
	unsigned long alg = cs->algorithms;
	EVP_PKEY *pkey = NULL;
	int keysize = 0;
	int signature_nid = 0;

	if (SSL_C_IS_EXPORT(cs))
		{
		/* ECDH key length in export ciphers must be <= 163 bits */
		pkey = X509_get_pubkey(x);
		if (pkey == NULL) return 0;
		keysize = EVP_PKEY_bits(pkey);
		EVP_PKEY_free(pkey);
		if (keysize > 163) return 0;
		}

	/* This call populates the ex_flags field correctly */
	X509_check_purpose(x, -1, 0);
	if ((x->sig_alg) && (x->sig_alg->algorithm))
		signature_nid = OBJ_obj2nid(x->sig_alg->algorithm);
	if (alg & SSL_kECDH) 
		{
		/* key usage, if present, must allow key agreement */
		if (ku_reject(x, X509v3_KU_KEY_AGREEMENT))
			{
			return 0;
			}
		if (alg & SSL_aECDSA) 
			{
			/* signature alg must be ECDSA */
			if (signature_nid != NID_ecdsa_with_SHA1)
				{
				return 0;
				}
			}
		if (alg & SSL_aRSA)
			{
			/* signature alg must be RSA */
			if ((signature_nid != NID_md5WithRSAEncryption) &&
			    (signature_nid != NID_md4WithRSAEncryption) &&
			    (signature_nid != NID_md2WithRSAEncryption))
				{
				return 0;
				}
			}
		} 
	else if (alg & SSL_aECDSA)
		{
		/* key usage, if present, must allow signing */
		if (ku_reject(x, X509v3_KU_DIGITAL_SIGNATURE))
			{
			return 0;
			}
		}

	return 1;  /* all checks are ok */
	}

/* THIS NEEDS CLEANING UP */
CERT_PKEY *ssl_get_server_send_pkey(const SSL *s)
	{
	unsigned long alg,kalg;
	CERT *c;
	int i;

	c=s->cert;
	ssl_set_cert_masks(c, s->s3->tmp.new_cipher);
	alg=s->s3->tmp.new_cipher->algorithms;
	kalg=alg&(SSL_MKEY_MASK|SSL_AUTH_MASK);

	if (kalg & SSL_kECDH)
		{
		/* we don't need to look at SSL_kECDHE 
		 * since no certificate is needed for
		 * anon ECDH and for authenticated
		 * ECDHE, the check for the auth 
		 * algorithm will set i correctly
		 * NOTE: For ECDH-RSA, we need an ECC
		 * not an RSA cert but for ECDHE-RSA
		 * we need an RSA cert. Placing the
		 * checks for SSL_kECDH before RSA
		 * checks ensures the correct cert is chosen.
		 */
		i=SSL_PKEY_ECC;
		}
	else if (kalg & SSL_aECDSA)
		{
		i=SSL_PKEY_ECC;
		}
	else if (kalg & SSL_kDHr)
		i=SSL_PKEY_DH_RSA;
	else if (kalg & SSL_kDHd)
		i=SSL_PKEY_DH_DSA;
	else if (kalg & SSL_aDSS)
		i=SSL_PKEY_DSA_SIGN;
	else if (kalg & SSL_aRSA)
		{
		if (c->pkeys[SSL_PKEY_RSA_ENC].x509 == NULL)
			i=SSL_PKEY_RSA_SIGN;
		else
			i=SSL_PKEY_RSA_ENC;
		}
	else if (kalg & SSL_aKRB5)
		{
		/* VRS something else here? */
		return(NULL);
		}
	else /* if (kalg & SSL_aNULL) */
		{
		SSLerr(SSL_F_SSL_GET_SERVER_SEND_PKEY,ERR_R_INTERNAL_ERROR);
		return(NULL);
		}

	return c->pkeys + i;
	}

X509 *ssl_get_server_send_cert(const SSL *s)
	{
	CERT_PKEY *cpk;
	cpk = ssl_get_server_send_pkey(s);
	if (!cpk)
		return NULL;
	return cpk->x509;
	}

EVP_PKEY *ssl_get_sign_pkey(SSL *s,SSL_CIPHER *cipher)
	{
	unsigned long alg;
	CERT *c;

	alg=cipher->algorithms;
	c=s->cert;

	if ((alg & SSL_aDSS) &&
		(c->pkeys[SSL_PKEY_DSA_SIGN].privatekey != NULL))
		return(c->pkeys[SSL_PKEY_DSA_SIGN].privatekey);
	else if (alg & SSL_aRSA)
		{
		if (c->pkeys[SSL_PKEY_RSA_SIGN].privatekey != NULL)
			return(c->pkeys[SSL_PKEY_RSA_SIGN].privatekey);
		else if (c->pkeys[SSL_PKEY_RSA_ENC].privatekey != NULL)
			return(c->pkeys[SSL_PKEY_RSA_ENC].privatekey);
		else
			return(NULL);
		}
	else if ((alg & SSL_aECDSA) &&
	         (c->pkeys[SSL_PKEY_ECC].privatekey != NULL))
		return(c->pkeys[SSL_PKEY_ECC].privatekey);
	else /* if (alg & SSL_aNULL) */
		{
		SSLerr(SSL_F_SSL_GET_SIGN_PKEY,ERR_R_INTERNAL_ERROR);
		return(NULL);
		}
	}

void ssl_update_cache(SSL *s,int mode)
	{
	int i;

	/* If the session_id_length is 0, we are not supposed to cache it,
	 * and it would be rather hard to do anyway :-) */
	if (s->session->session_id_length == 0) return;

	i=s->ctx->session_cache_mode;
	if ((i & mode) && (!s->hit)
		&& ((i & SSL_SESS_CACHE_NO_INTERNAL_STORE)
		    || SSL_CTX_add_session(s->ctx,s->session))
		&& (s->ctx->new_session_cb != NULL))
		{
		CRYPTO_add(&s->session->references,1,CRYPTO_LOCK_SSL_SESSION);
		if (!s->ctx->new_session_cb(s,s->session))
			SSL_SESSION_free(s->session);
		}

	/* auto flush every 255 connections */
	if ((!(i & SSL_SESS_CACHE_NO_AUTO_CLEAR)) &&
		((i & mode) == mode))
		{
		if (  (((mode & SSL_SESS_CACHE_CLIENT)
			?s->ctx->stats.sess_connect_good
			:s->ctx->stats.sess_accept_good) & 0xff) == 0xff)
			{
			SSL_CTX_flush_sessions(s->ctx,(unsigned long)time(NULL));
			}
		}
	}

SSL_METHOD *SSL_get_ssl_method(SSL *s)
	{
	return(s->method);
	}

int SSL_set_ssl_method(SSL *s,SSL_METHOD *meth)
	{
	int conn= -1;
	int ret=1;

	if (s->method != meth)
		{
		if (s->handshake_func != NULL)
			conn=(s->handshake_func == s->method->ssl_connect);

		if (s->method->version == meth->version)
			s->method=meth;
		else
			{
			s->method->ssl_free(s);
			s->method=meth;
			ret=s->method->ssl_new(s);
			}

		if (conn == 1)
			s->handshake_func=meth->ssl_connect;
		else if (conn == 0)
			s->handshake_func=meth->ssl_accept;
		}
	return(ret);
	}

int SSL_get_error(const SSL *s,int i)
	{
	int reason;
	unsigned long l;
	BIO *bio;

	if (i > 0) return(SSL_ERROR_NONE);

	/* Make things return SSL_ERROR_SYSCALL when doing SSL_do_handshake
	 * etc, where we do encode the error */
	if ((l=ERR_peek_error()) != 0)
		{
		if (ERR_GET_LIB(l) == ERR_LIB_SYS)
			return(SSL_ERROR_SYSCALL);
		else
			return(SSL_ERROR_SSL);
		}

	if ((i < 0) && SSL_want_read(s))
		{
		bio=SSL_get_rbio(s);
		if (BIO_should_read(bio))
			return(SSL_ERROR_WANT_READ);
		else if (BIO_should_write(bio))
			/* This one doesn't make too much sense ... We never try
			 * to write to the rbio, and an application program where
			 * rbio and wbio are separate couldn't even know what it
			 * should wait for.
			 * However if we ever set s->rwstate incorrectly
			 * (so that we have SSL_want_read(s) instead of
			 * SSL_want_write(s)) and rbio and wbio *are* the same,
			 * this test works around that bug; so it might be safer
			 * to keep it. */
			return(SSL_ERROR_WANT_WRITE);
		else if (BIO_should_io_special(bio))
			{
			reason=BIO_get_retry_reason(bio);
			if (reason == BIO_RR_CONNECT)
				return(SSL_ERROR_WANT_CONNECT);
			else if (reason == BIO_RR_ACCEPT)
				return(SSL_ERROR_WANT_ACCEPT);
			else
				return(SSL_ERROR_SYSCALL); /* unknown */
			}
		}

	if ((i < 0) && SSL_want_write(s))
		{
		bio=SSL_get_wbio(s);
		if (BIO_should_write(bio))
			return(SSL_ERROR_WANT_WRITE);
		else if (BIO_should_read(bio))
			/* See above (SSL_want_read(s) with BIO_should_write(bio)) */
			return(SSL_ERROR_WANT_READ);
		else if (BIO_should_io_special(bio))
			{
			reason=BIO_get_retry_reason(bio);
			if (reason == BIO_RR_CONNECT)
				return(SSL_ERROR_WANT_CONNECT);
			else if (reason == BIO_RR_ACCEPT)
				return(SSL_ERROR_WANT_ACCEPT);
			else
				return(SSL_ERROR_SYSCALL);
			}
		}
	if ((i < 0) && SSL_want_x509_lookup(s))
		{
		return(SSL_ERROR_WANT_X509_LOOKUP);
		}

	if (i == 0)
		{
		if (s->version == SSL2_VERSION)
			{
			/* assume it is the socket being closed */
			return(SSL_ERROR_ZERO_RETURN);
			}
		else
			{
			if ((s->shutdown & SSL_RECEIVED_SHUTDOWN) &&
				(s->s3->warn_alert == SSL_AD_CLOSE_NOTIFY))
				return(SSL_ERROR_ZERO_RETURN);
			}
		}
	return(SSL_ERROR_SYSCALL);
	}

int SSL_do_handshake(SSL *s)
	{
	int ret=1;

	if (s->handshake_func == NULL)
		{
		SSLerr(SSL_F_SSL_DO_HANDSHAKE,SSL_R_CONNECTION_TYPE_NOT_SET);
		return(-1);
		}

	s->method->ssl_renegotiate_check(s);

	if (SSL_in_init(s) || SSL_in_before(s))
		{
		ret=s->handshake_func(s);
		}
	return(ret);
	}

/* For the next 2 functions, SSL_clear() sets shutdown and so
 * one of these calls will reset it */
void SSL_set_accept_state(SSL *s)
	{
	s->server=1;
	s->shutdown=0;
	s->state=SSL_ST_ACCEPT|SSL_ST_BEFORE;
	s->handshake_func=s->method->ssl_accept;
	/* clear the current cipher */
	ssl_clear_cipher_ctx(s);
	}

void SSL_set_connect_state(SSL *s)
	{
	s->server=0;
	s->shutdown=0;
	s->state=SSL_ST_CONNECT|SSL_ST_BEFORE;
	s->handshake_func=s->method->ssl_connect;
	/* clear the current cipher */
	ssl_clear_cipher_ctx(s);
	}

int ssl_undefined_function(SSL *s)
	{
	SSLerr(SSL_F_SSL_UNDEFINED_FUNCTION,ERR_R_SHOULD_NOT_HAVE_BEEN_CALLED);
	return(0);
	}

int ssl_undefined_void_function(void)
	{
	SSLerr(SSL_F_SSL_UNDEFINED_VOID_FUNCTION,ERR_R_SHOULD_NOT_HAVE_BEEN_CALLED);
	return(0);
	}

int ssl_undefined_const_function(const SSL *s)
	{
	SSLerr(SSL_F_SSL_UNDEFINED_CONST_FUNCTION,ERR_R_SHOULD_NOT_HAVE_BEEN_CALLED);
	return(0);
	}

SSL_METHOD *ssl_bad_method(int ver)
	{
	SSLerr(SSL_F_SSL_BAD_METHOD,ERR_R_SHOULD_NOT_HAVE_BEEN_CALLED);
	return(NULL);
	}

const char *SSL_get_version(const SSL *s)
	{
	if (s->version == TLS1_VERSION)
		return("TLSv1");
	else if (s->version == SSL3_VERSION)
		return("SSLv3");
	else if (s->version == SSL2_VERSION)
		return("SSLv2");
	else
		return("unknown");
	}

SSL *SSL_dup(SSL *s)
	{
	STACK_OF(X509_NAME) *sk;
	X509_NAME *xn;
	SSL *ret;
	int i;
		 
	if ((ret=SSL_new(SSL_get_SSL_CTX(s))) == NULL)
	    return(NULL);

	ret->version = s->version;
	ret->type = s->type;
	ret->method = s->method;

	if (s->session != NULL)
		{
		/* This copies session-id, SSL_METHOD, sid_ctx, and 'cert' */
		SSL_copy_session_id(ret,s);
		}
	else
		{
		/* No session has been established yet, so we have to expect
		 * that s->cert or ret->cert will be changed later --
		 * they should not both point to the same object,
		 * and thus we can't use SSL_copy_session_id. */

		ret->method->ssl_free(ret);
		ret->method = s->method;
		ret->method->ssl_new(ret);

		if (s->cert != NULL)
			{
			if (ret->cert != NULL)
				{
				ssl_cert_free(ret->cert);
				}
			ret->cert = ssl_cert_dup(s->cert);
			if (ret->cert == NULL)
				goto err;
			}
				
		SSL_set_session_id_context(ret,
			s->sid_ctx, s->sid_ctx_length);
		}

	ret->options=s->options;
	ret->mode=s->mode;
	SSL_set_max_cert_list(ret,SSL_get_max_cert_list(s));
	SSL_set_read_ahead(ret,SSL_get_read_ahead(s));
	ret->msg_callback = s->msg_callback;
	ret->msg_callback_arg = s->msg_callback_arg;
	SSL_set_verify(ret,SSL_get_verify_mode(s),
		SSL_get_verify_callback(s));
	SSL_set_verify_depth(ret,SSL_get_verify_depth(s));
	ret->generate_session_id = s->generate_session_id;

	SSL_set_info_callback(ret,SSL_get_info_callback(s));
	
	ret->debug=s->debug;

	/* copy app data, a little dangerous perhaps */
	if (!CRYPTO_dup_ex_data(CRYPTO_EX_INDEX_SSL, &ret->ex_data, &s->ex_data))
		goto err;

	/* setup rbio, and wbio */
	if (s->rbio != NULL)
		{
		if (!BIO_dup_state(s->rbio,(char *)&ret->rbio))
			goto err;
		}
	if (s->wbio != NULL)
		{
		if (s->wbio != s->rbio)
			{
			if (!BIO_dup_state(s->wbio,(char *)&ret->wbio))
				goto err;
			}
		else
			ret->wbio=ret->rbio;
		}
	ret->rwstate = s->rwstate;
	ret->in_handshake = s->in_handshake;
	ret->handshake_func = s->handshake_func;
	ret->server = s->server;
	ret->new_session = s->new_session;
	ret->quiet_shutdown = s->quiet_shutdown;
	ret->shutdown=s->shutdown;
	ret->state=s->state; /* SSL_dup does not really work at any state, though */
	ret->rstate=s->rstate;
	ret->init_num = 0; /* would have to copy ret->init_buf, ret->init_msg, ret->init_num, ret->init_off */
	ret->hit=s->hit;

	X509_VERIFY_PARAM_inherit(ret->param, s->param);

	/* dup the cipher_list and cipher_list_by_id stacks */
	if (s->cipher_list != NULL)
		{
		if ((ret->cipher_list=sk_SSL_CIPHER_dup(s->cipher_list)) == NULL)
			goto err;
		}
	if (s->cipher_list_by_id != NULL)
		if ((ret->cipher_list_by_id=sk_SSL_CIPHER_dup(s->cipher_list_by_id))
			== NULL)
			goto err;

	/* Dup the client_CA list */
	if (s->client_CA != NULL)
		{
		if ((sk=sk_X509_NAME_dup(s->client_CA)) == NULL) goto err;
		ret->client_CA=sk;
		for (i=0; i<sk_X509_NAME_num(sk); i++)
			{
			xn=sk_X509_NAME_value(sk,i);
			if (sk_X509_NAME_set(sk,i,X509_NAME_dup(xn)) == NULL)
				{
				X509_NAME_free(xn);
				goto err;
				}
			}
		}

	if (0)
		{
err:
		if (ret != NULL) SSL_free(ret);
		ret=NULL;
		}
	return(ret);
	}

void ssl_clear_cipher_ctx(SSL *s)
	{
	if (s->enc_read_ctx != NULL)
		{
		EVP_CIPHER_CTX_cleanup(s->enc_read_ctx);
		OPENSSL_free(s->enc_read_ctx);
		s->enc_read_ctx=NULL;
		}
	if (s->enc_write_ctx != NULL)
		{
		EVP_CIPHER_CTX_cleanup(s->enc_write_ctx);
		OPENSSL_free(s->enc_write_ctx);
		s->enc_write_ctx=NULL;
		}
#ifndef OPENSSL_NO_COMP
	if (s->expand != NULL)
		{
		COMP_CTX_free(s->expand);
		s->expand=NULL;
		}
	if (s->compress != NULL)
		{
		COMP_CTX_free(s->compress);
		s->compress=NULL;
		}
#endif
	}

/* Fix this function so that it takes an optional type parameter */
X509 *SSL_get_certificate(const SSL *s)
	{
	if (s->cert != NULL)
		return(s->cert->key->x509);
	else
		return(NULL);
	}

/* Fix this function so that it takes an optional type parameter */
EVP_PKEY *SSL_get_privatekey(SSL *s)
	{
	if (s->cert != NULL)
		return(s->cert->key->privatekey);
	else
		return(NULL);
	}

SSL_CIPHER *SSL_get_current_cipher(const SSL *s)
	{
	if ((s->session != NULL) && (s->session->cipher != NULL))
		return(s->session->cipher);
	return(NULL);
	}
#ifdef OPENSSL_NO_COMP
const void *SSL_get_current_compression(SSL *s)
	{
	return NULL;
	}
const void *SSL_get_current_expansion(SSL *s)
	{
	return NULL;
	}
#else

const COMP_METHOD *SSL_get_current_compression(SSL *s)
	{
	if (s->compress != NULL)
		return(s->compress->meth);
	return(NULL);
	}

const COMP_METHOD *SSL_get_current_expansion(SSL *s)
	{
	if (s->expand != NULL)
		return(s->expand->meth);
	return(NULL);
	}
#endif

int ssl_init_wbio_buffer(SSL *s,int push)
	{
	BIO *bbio;

	if (s->bbio == NULL)
		{
		bbio=BIO_new(BIO_f_buffer());
		if (bbio == NULL) return(0);
		s->bbio=bbio;
		}
	else
		{
		bbio=s->bbio;
		if (s->bbio == s->wbio)
			s->wbio=BIO_pop(s->wbio);
		}
	(void)BIO_reset(bbio);
/*	if (!BIO_set_write_buffer_size(bbio,16*1024)) */
	if (!BIO_set_read_buffer_size(bbio,1))
		{
		SSLerr(SSL_F_SSL_INIT_WBIO_BUFFER,ERR_R_BUF_LIB);
		return(0);
		}
	if (push)
		{
		if (s->wbio != bbio)
			s->wbio=BIO_push(bbio,s->wbio);
		}
	else
		{
		if (s->wbio == bbio)
			s->wbio=BIO_pop(bbio);
		}
	return(1);
	}

void ssl_free_wbio_buffer(SSL *s)
	{
	if (s->bbio == NULL) return;

	if (s->bbio == s->wbio)
		{
		/* remove buffering */
		s->wbio=BIO_pop(s->wbio);
#ifdef REF_CHECK /* not the usual REF_CHECK, but this avoids adding one more preprocessor symbol */
		assert(s->wbio != NULL);
#endif	
	}
	BIO_free(s->bbio);
	s->bbio=NULL;
	}
	
void SSL_CTX_set_quiet_shutdown(SSL_CTX *ctx,int mode)
	{
	ctx->quiet_shutdown=mode;
	}

int SSL_CTX_get_quiet_shutdown(const SSL_CTX *ctx)
	{
	return(ctx->quiet_shutdown);
	}

void SSL_set_quiet_shutdown(SSL *s,int mode)
	{
	s->quiet_shutdown=mode;
	}

int SSL_get_quiet_shutdown(const SSL *s)
	{
	return(s->quiet_shutdown);
	}

void SSL_set_shutdown(SSL *s,int mode)
	{
	s->shutdown=mode;
	}

int SSL_get_shutdown(const SSL *s)
	{
	return(s->shutdown);
	}

int SSL_version(const SSL *s)
	{
	return(s->version);
	}

SSL_CTX *SSL_get_SSL_CTX(const SSL *ssl)
	{
	return(ssl->ctx);
	}

SSL_CTX *SSL_set_SSL_CTX(SSL *ssl, SSL_CTX* ctx)
	{
	if (ssl->ctx == ctx)
		return ssl->ctx;
#ifndef OPENSSL_NO_TLSEXT
	if (ctx == NULL)
		ctx = ssl->initial_ctx;
#endif
	if (ssl->cert != NULL)
		ssl_cert_free(ssl->cert);
	ssl->cert = ssl_cert_dup(ctx->cert);
	CRYPTO_add(&ctx->references,1,CRYPTO_LOCK_SSL_CTX);
	if (ssl->ctx != NULL)
		SSL_CTX_free(ssl->ctx); /* decrement reference count */
	ssl->ctx = ctx;
	return(ssl->ctx);
	}

#ifndef OPENSSL_NO_STDIO
int SSL_CTX_set_default_verify_paths(SSL_CTX *ctx)
	{
	return(X509_STORE_set_default_paths(ctx->cert_store));
	}

int SSL_CTX_load_verify_locations(SSL_CTX *ctx, const char *CAfile,
		const char *CApath)
	{
	return(X509_STORE_load_locations(ctx->cert_store,CAfile,CApath));
	}
#endif

void SSL_set_info_callback(SSL *ssl,
	void (*cb)(const SSL *ssl,int type,int val))
	{
	ssl->info_callback=cb;
	}

/* One compiler (Diab DCC) doesn't like argument names in returned
   function pointer.  */
void (*SSL_get_info_callback(const SSL *ssl))(const SSL * /*ssl*/,int /*type*/,int /*val*/) 
	{
	return ssl->info_callback;
	}

int SSL_state(const SSL *ssl)
	{
	return(ssl->state);
	}

void SSL_set_verify_result(SSL *ssl,long arg)
	{
	ssl->verify_result=arg;
	}

long SSL_get_verify_result(const SSL *ssl)
	{
	return(ssl->verify_result);
	}

int SSL_get_ex_new_index(long argl,void *argp,CRYPTO_EX_new *new_func,
			 CRYPTO_EX_dup *dup_func,CRYPTO_EX_free *free_func)
	{
	return CRYPTO_get_ex_new_index(CRYPTO_EX_INDEX_SSL, argl, argp,
				new_func, dup_func, free_func);
	}

int SSL_set_ex_data(SSL *s,int idx,void *arg)
	{
	return(CRYPTO_set_ex_data(&s->ex_data,idx,arg));
	}

void *SSL_get_ex_data(const SSL *s,int idx)
	{
	return(CRYPTO_get_ex_data(&s->ex_data,idx));
	}

int SSL_CTX_get_ex_new_index(long argl,void *argp,CRYPTO_EX_new *new_func,
			     CRYPTO_EX_dup *dup_func,CRYPTO_EX_free *free_func)
	{
	return CRYPTO_get_ex_new_index(CRYPTO_EX_INDEX_SSL_CTX, argl, argp,
				new_func, dup_func, free_func);
	}

int SSL_CTX_set_ex_data(SSL_CTX *s,int idx,void *arg)
	{
	return(CRYPTO_set_ex_data(&s->ex_data,idx,arg));
	}

void *SSL_CTX_get_ex_data(const SSL_CTX *s,int idx)
	{
	return(CRYPTO_get_ex_data(&s->ex_data,idx));
	}

int ssl_ok(SSL *s)
	{
	return(1);
	}

X509_STORE *SSL_CTX_get_cert_store(const SSL_CTX *ctx)
	{
	return(ctx->cert_store);
	}

void SSL_CTX_set_cert_store(SSL_CTX *ctx,X509_STORE *store)
	{
	if (ctx->cert_store != NULL)
		X509_STORE_free(ctx->cert_store);
	ctx->cert_store=store;
	}

int SSL_want(const SSL *s)
	{
	return(s->rwstate);
	}

/*!
 * \brief Set the callback for generating temporary RSA keys.
 * \param ctx the SSL context.
 * \param cb the callback
 */

#ifndef OPENSSL_NO_RSA
void SSL_CTX_set_tmp_rsa_callback(SSL_CTX *ctx,RSA *(*cb)(SSL *ssl,
							  int is_export,
							  int keylength))
    {
    SSL_CTX_callback_ctrl(ctx,SSL_CTRL_SET_TMP_RSA_CB,(void (*)(void))cb);
    }

void SSL_set_tmp_rsa_callback(SSL *ssl,RSA *(*cb)(SSL *ssl,
						  int is_export,
						  int keylength))
    {
    SSL_callback_ctrl(ssl,SSL_CTRL_SET_TMP_RSA_CB,(void (*)(void))cb);
    }
#endif

#ifdef DOXYGEN
/*!
 * \brief The RSA temporary key callback function.
 * \param ssl the SSL session.
 * \param is_export \c TRUE if the temp RSA key is for an export ciphersuite.
 * \param keylength if \c is_export is \c TRUE, then \c keylength is the size
 * of the required key in bits.
 * \return the temporary RSA key.
 * \sa SSL_CTX_set_tmp_rsa_callback, SSL_set_tmp_rsa_callback
 */

RSA *cb(SSL *ssl,int is_export,int keylength)
    {}
#endif

/*!
 * \brief Set the callback for generating temporary DH keys.
 * \param ctx the SSL context.
 * \param dh the callback
 */

#ifndef OPENSSL_NO_DH
void SSL_CTX_set_tmp_dh_callback(SSL_CTX *ctx,DH *(*dh)(SSL *ssl,int is_export,
							int keylength))
	{
	SSL_CTX_callback_ctrl(ctx,SSL_CTRL_SET_TMP_DH_CB,(void (*)(void))dh);
	}

void SSL_set_tmp_dh_callback(SSL *ssl,DH *(*dh)(SSL *ssl,int is_export,
						int keylength))
	{
	SSL_callback_ctrl(ssl,SSL_CTRL_SET_TMP_DH_CB,(void (*)(void))dh);
	}
#endif

#ifndef OPENSSL_NO_ECDH
void SSL_CTX_set_tmp_ecdh_callback(SSL_CTX *ctx,EC_KEY *(*ecdh)(SSL *ssl,int is_export,
							int keylength))
	{
	SSL_CTX_callback_ctrl(ctx,SSL_CTRL_SET_TMP_ECDH_CB,(void (*)(void))ecdh);
	}

void SSL_set_tmp_ecdh_callback(SSL *ssl,EC_KEY *(*ecdh)(SSL *ssl,int is_export,
						int keylength))
	{
	SSL_callback_ctrl(ssl,SSL_CTRL_SET_TMP_ECDH_CB,(void (*)(void))ecdh);
	}
#endif


void SSL_CTX_set_msg_callback(SSL_CTX *ctx, void (*cb)(int write_p, int version, int content_type, const void *buf, size_t len, SSL *ssl, void *arg))
	{
	SSL_CTX_callback_ctrl(ctx, SSL_CTRL_SET_MSG_CALLBACK, (void (*)(void))cb);
	}
void SSL_set_msg_callback(SSL *ssl, void (*cb)(int write_p, int version, int content_type, const void *buf, size_t len, SSL *ssl, void *arg))
	{
	SSL_callback_ctrl(ssl, SSL_CTRL_SET_MSG_CALLBACK, (void (*)(void))cb);
	}



#if defined(_WINDLL) && defined(OPENSSL_SYS_WIN16)
#include "../crypto/bio/bss_file.c"
#endif

IMPLEMENT_STACK_OF(SSL_CIPHER)
IMPLEMENT_STACK_OF(SSL_COMP)
