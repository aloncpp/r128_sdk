#ifdef AW_CYGWIN

extern const char _ctype_;

const char *__locale_ctype_ptr_l (struct __locale_t *locale)
{
	return &_ctype_;
}

const char *__locale_ctype_ptr (void)
{
	return &_ctype_;
}

#else

static int dumy_ctype(void)
{
    return 0;
}

#endif
