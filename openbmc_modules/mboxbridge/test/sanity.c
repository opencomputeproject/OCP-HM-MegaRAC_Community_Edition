// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2018 IBM Corp.
int main(void)
{
#ifdef NDEBUG
	return 1;
#else
	return 0;
#endif
}
