/*
 *         __   _____    ______   ______   ___    ___
 *        /\ \ /\  _ `\ /\  ___\ /\  _  \ /\_ \  /\_ \
 *        \ \ \\ \ \L\ \\ \ \__/ \ \ \L\ \\//\ \ \//\ \      __     __
 *      __ \ \ \\ \  __| \ \ \  __\ \  __ \ \ \ \  \ \ \   /'__`\ /'_ `\
 *     /\ \_\/ / \ \ \/   \ \ \L\ \\ \ \/\ \ \_\ \_ \_\ \_/\  __//\ \L\ \
 *     \ \____//  \ \_\    \ \____/ \ \_\ \_\/\____\/\____\ \____\ \____ \
 *      \/____/    \/_/     \/___/   \/_/\/_/\/____/\/____/\/____/\/___L\ \
 *                                                                  /\____/
 *                                                                  \_/__/
 *
 *      Version 2.6, by Angelo Mottola, 2000-2006
 *
 *      Decoder core module.
 *
 *      See the readme.txt file for instructions on using this package in your
 *      own programs.
 */


#include <internal.h>


static HUFFMAN_TABLE *ac_luminance_table, *dc_luminance_table;
static HUFFMAN_TABLE *ac_chrominance_table, *dc_chrominance_table;
static DATA_BUFFER *data_buffer[3];
static short quantization_table[256];
static short *luminance_quantization_table, *chrominance_quantization_table;
static int jpeg_w, jpeg_h, jpeg_components;
static int sampling, v_sampling, h_sampling, restart_interval, skip_count;
static int spectrum_start, spectrum_end, successive_high, successive_low;
static int scan_components, component[3];
static int progress_counter, progress_total;
static void (*idct)(short *block, short *dequant, short *output, short *workspace);
static void (*ycbcr2rgb)(intptr_t address, int y1, int cb1, int cr1, int y2, int cb2, int cr2, int y3, int cb3, int cr3, int y4, int cb4, int cr4);
static void (*plot)(intptr_t addr, int pitch, short *y1, short *y2, short *y3, short *y4, short *cb, short *cr);
static void (*progress_cb)(int percentage);



/* _jpeg_c_idct:
 *  Applies the inverse discrete cosine transform to the given input data,
 *  in the form of a vector of 64 coefficients.
 *  This uses integer fixed point math and is based on code by the IJG.
 */
static void
_jpeg_c_idct(short *data, short *output, short *dequant, short *workspace)
{
	int tmp0, tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7;
	int tmp10, tmp11, tmp12, tmp13;
	int z5, z10, z11, z12, z13;
	short *inptr, *dqptr, *outptr;
	int *wsptr;
	int i, temp;
	
	inptr = data;
	dqptr = dequant;
	wsptr = (int *)workspace;
	for (i = 8; i; i--) {
		if ((inptr[8] | inptr[16] | inptr[24] | inptr[32] | inptr[40] | inptr[48] | inptr[56]) == 0) {
			temp = inptr[0] * dqptr[0];
			wsptr[0] = temp;
			wsptr[8] = temp;
			wsptr[16] = temp;
			wsptr[24] = temp;
			wsptr[32] = temp;
			wsptr[40] = temp;
			wsptr[48] = temp;
			wsptr[56] = temp;
			inptr++;
			dqptr++;
			wsptr++;
			continue;
		}
		tmp0 = inptr[0] * dqptr[0];
		tmp1 = inptr[16] * dqptr[16];
		tmp2 = inptr[32] * dqptr[32];
		tmp3 = inptr[48] * dqptr[48];
		tmp10 = tmp0 + tmp2;
		tmp11 = tmp0 - tmp2;
		tmp13 = tmp1 + tmp3;
		tmp12 = (((tmp1 - tmp3) * IFIX_1_414213562) >> 8) - tmp13;
		tmp0 = tmp10 + tmp13;
		tmp3 = tmp10 - tmp13;
		tmp1 = tmp11 + tmp12;
		tmp2 = tmp11 - tmp12;
		tmp4 = inptr[8] * dqptr[8];
		tmp5 = inptr[24] * dqptr[24];
		tmp6 = inptr[40] * dqptr[40];
		tmp7 = inptr[56] * dqptr[56];
		z13 = tmp6 + tmp5;
		z10 = tmp6 - tmp5;
		z11 = tmp4 + tmp7;
		z12 = tmp4 - tmp7;
		tmp7 = z11 + z13;
		tmp11 = ((z11 - z13) * IFIX_1_414213562) >> 8;
		z5 = ((z10 + z12) * IFIX_1_847759065) >> 8;
		tmp10 = ((z12 * IFIX_1_082392200) >> 8) - z5;
		tmp12 = ((z10 * -IFIX_2_613125930) >> 8) + z5;
		tmp6 = tmp12 - tmp7;
		tmp5 = tmp11 - tmp6;
		tmp4 = tmp10 + tmp5;
		wsptr[0] = tmp0 + tmp7;
		wsptr[56] = tmp0 - tmp7;
		wsptr[8] = tmp1 + tmp6;
		wsptr[48] = tmp1 - tmp6;
		wsptr[16] = tmp2 + tmp5;
		wsptr[40] = tmp2 - tmp5;
		wsptr[32] = tmp3 + tmp4;
		wsptr[24] = tmp3 - tmp4;
		inptr++;
		dqptr++;
		wsptr++;
	}
	
	wsptr = (int *)workspace;
	outptr = output;
	for (i = 8; i; i--) {
		tmp10 = wsptr[0] + wsptr[4];
		tmp11 = wsptr[0] - wsptr[4];
		tmp13 = wsptr[2] + wsptr[6];
		tmp12 = (((wsptr[2] - wsptr[6]) * IFIX_1_414213562) >> 8) - tmp13;
		tmp0 = tmp10 + tmp13;
		tmp3 = tmp10 - tmp13;
		tmp1 = tmp11 + tmp12;
		tmp2 = tmp11 - tmp12;
		z13 = wsptr[5] + wsptr[3];
		z10 = wsptr[5] - wsptr[3];
		z11 = wsptr[1] + wsptr[7];
		z12 = wsptr[1] - wsptr[7];
		tmp7 = z11 + z13;
		tmp11 = ((z11 - z13) * IFIX_1_414213562) >> 8;
		z5 = ((z10 + z12) * IFIX_1_847759065) >> 8;
		tmp10 = ((z12 * IFIX_1_082392200) >> 8) - z5;
		tmp12 = ((z10 * -IFIX_2_613125930) >> 8) + z5;
		tmp6 = tmp12 - tmp7;
		tmp5 = tmp11 - tmp6;
		tmp4 = tmp10 + tmp5;
		outptr[0] = ((tmp0 + tmp7) >> 5) + 128;
		outptr[7] = ((tmp0 - tmp7) >> 5) + 128;
		outptr[1] = ((tmp1 + tmp6) >> 5) + 128;
		outptr[6] = ((tmp1 - tmp6) >> 5) + 128;
		outptr[2] = ((tmp2 + tmp5) >> 5) + 128;
		outptr[5] = ((tmp2 - tmp5) >> 5) + 128;
		outptr[4] = ((tmp3 + tmp4) >> 5) + 128;
		outptr[3] = ((tmp3 - tmp4) >> 5) + 128;
		wsptr += 8;
		outptr += 8;
	}
}


/* zigzag_reorder:
 *  Reorders a vector of 64 coefficients by the zigzag scan.
 */
static INLINE void
zigzag_reorder(short *input, short *output)
{
	int i;
	
	for (i = 0; i < 64; i++)
		output[i] = input[_jpeg_zigzag_scan[i]];
}


/* free_huffman_table:
 *  Frees memory used by the huffman decoding lookup tables.
 */
static void
free_huffman_table(HUFFMAN_TABLE *table)
{
	int i;
	
	for (i = 0; i < 16; i++) {
		if (table->entry_of_length[i])
			free(table->entry_of_length[i]);
		table->entry_of_length[i] = NULL;
	}
}


/* read_dht_chunk:
 *  Reads a DHT (Define Huffman Table) chunk from the input stream.
 */
static int
read_dht_chunk(void)
{
	int i, j, table_id, num_codes[16];
	int code, value;
	unsigned char data;
	HUFFMAN_TABLE *table;
	HUFFMAN_ENTRY *entry;
	
	_jpeg_open_chunk();
	do {
		data = _jpeg_getc();
		if (data & 0xe0) {
			TRACE("Invalid DHT information byte");
			jpgalleg_error = JPG_ERROR_BAD_IMAGE;
			return -1;
		}
		table_id = data & 0xf;
		if (table_id > 3) {
			TRACE("Invalid huffman table number");
			jpgalleg_error = JPG_ERROR_BAD_IMAGE;
			return -1;
		}
		if (data & 0x10)
			table = &_jpeg_huffman_ac_table[table_id];
		else
			table = &_jpeg_huffman_dc_table[table_id];
		for (i = 0; i < 16; i++)
			num_codes[i] = _jpeg_getc();
		code = 0;
		for (i = 0; i < 16; i++) {
			if (table->entry_of_length[i])
				free(table->entry_of_length[i]);
			table->entry_of_length[i] = (HUFFMAN_ENTRY *)calloc(1 << (i + 1), sizeof(HUFFMAN_ENTRY));
			if (!table->entry_of_length[i]) {
				TRACE("Out of memory");
				jpgalleg_error = JPG_ERROR_OUT_OF_MEMORY;
				return -1;
			}
			for (j = 0; j < num_codes[i]; j++) {
				value = _jpeg_getc();
				entry = &table->entry_of_length[i][code];
				entry->value = value;
				entry->encoded_value = code;
				entry->bits_length = i + 1;
				code++;
			}
			code <<= 1;
		}
	} while (!_jpeg_eoc());
	_jpeg_close_chunk();
	
	return 0;
}


/* read_sof0_chunk:
 *  Reads a SOF0 (Start Of Frame 0) chunk from the input stream.
 */
static int
read_sof0_chunk(void)
{
	int i, data;
	
	_jpeg_open_chunk();
	if ((data = _jpeg_getc()) != 8) {
		TRACE("Unsupported data precision (%d)", data);
		jpgalleg_error = JPG_ERROR_UNSUPPORTED_DATA_PRECISION;
		return -1;
	}
	jpeg_h = _jpeg_getw();
	jpeg_w = _jpeg_getw();
	jpeg_components = _jpeg_getc();
	if ((jpeg_components != 1) && (jpeg_components != 3)) {
		TRACE("Unsupported number of components (%d)", jpeg_components);
		jpgalleg_error = JPG_ERROR_UNSUPPORTED_COLOR_SPACE;
		return -1;
	}
	for (i = 0; i < jpeg_components; i++) {
		switch (_jpeg_getc()) {
			case 1:
				data = _jpeg_getc();
				h_sampling = data >> 4;
				v_sampling = data & 0xf;
				sampling = h_sampling * v_sampling;
				if ((sampling != 1) && (sampling != 2) && (sampling != 4)) {
					TRACE("Bad sampling byte (%d)", sampling);
					jpgalleg_error = JPG_ERROR_BAD_IMAGE;
					return -1;
				}
				data = _jpeg_getc();
				if (data > 3) {
					TRACE("Bad quantization table number (%d)", data);
					jpgalleg_error = JPG_ERROR_BAD_IMAGE;
					return -1;
				}
				luminance_quantization_table = &quantization_table[data * 64];
				break;
			case 2:
			case 3:
				_jpeg_getc();
				data = _jpeg_getc();
				if (data > 3) {
					TRACE("Bad quantization table number (%d)", data);
					jpgalleg_error = JPG_ERROR_BAD_IMAGE;
					return -1;
				}
				chrominance_quantization_table = &quantization_table[data * 64];
				break;
		}
	}
	_jpeg_close_chunk();
	
	return 0;
}


/* read_dqt_chunk:
 *  Reads a DQT (Define Quantization Table) chunk from the input stream.
 */
static int
read_dqt_chunk(void)
{
	int i, data;
	short *table, temp[64];
	float value;
	
	_jpeg_open_chunk();
	do {
  		data = _jpeg_getc();
		if ((data & 0xf) > 3) {
			TRACE("Bad quantization table number (%d)", data);
			jpgalleg_error = JPG_ERROR_BAD_IMAGE;
			return -1;
		}
		if (data & 0xf0) {
			TRACE("Unsupported quantization table data precision");
			jpgalleg_error = JPG_ERROR_UNSUPPORTED_DATA_PRECISION;
			return -1;
		}
		table = &quantization_table[(data & 0xf) * 64];
		for (i = 0; i < 64; i++)
			temp[i] = _jpeg_getc();
		zigzag_reorder(temp, table);
		for (i = 0; i < 64; i++) {
			value = (float)table[i] * AAN_FACTOR(i) * 16384.0;
			table[i] = ((int)value + (1 << 11)) >> 12;
		}
	} while (!_jpeg_eoc());
	_jpeg_close_chunk();
	
	return 0;
}


/* read_sos_chunk:
 *  Reads a SOS (Start Of Scan) chunk from the input stream.
 */
static int
read_sos_chunk(void)
{
	int i, data;
	
	_jpeg_open_chunk();
	scan_components = _jpeg_getc();
	if (scan_components > 3) {
		TRACE("Unsupported number of scan components (%d)", scan_components);
		jpgalleg_error = JPG_ERROR_UNSUPPORTED_COLOR_SPACE;
		return -1;
	}
	for (i = 0; i < scan_components; i++) {
		component[i] = _jpeg_getc();
		switch (component[i]) {
			case 1:
				data = _jpeg_getc();
				if (((data & 0xf) > 3) || ((data >> 4) > 3)) {
					TRACE("Bad huffman table specified for %s component", _jpeg_component_name[component[i] - 1]);
					jpgalleg_error = JPG_ERROR_BAD_IMAGE;
					return -1;
				}
				ac_luminance_table = &_jpeg_huffman_ac_table[data & 0xf];
				dc_luminance_table = &_jpeg_huffman_dc_table[data >> 4];
				break;
			case 2:
			case 3:
				data = _jpeg_getc();
				if (((data & 0xf) > 3) || ((data >> 4) > 3)) {
					TRACE("Bad huffman table specified for %s component", _jpeg_component_name[component[i] - 1]);
					jpgalleg_error = JPG_ERROR_BAD_IMAGE;
					return -1;
				}
				ac_chrominance_table = &_jpeg_huffman_ac_table[data & 0xf];
				dc_chrominance_table = &_jpeg_huffman_dc_table[data >> 4];
				break;
			default:
				TRACE("Unsupported component id (%d)", component[i]);
				jpgalleg_error = JPG_ERROR_BAD_IMAGE;
				break;
		}
	}
	spectrum_start = _jpeg_getc();
	spectrum_end = _jpeg_getc();
	data = _jpeg_getc();
	successive_high = data >> 4;
	successive_low = data & 0xf;
	_jpeg_close_chunk();
	skip_count = 0;
	return 0;
}


/* read_appn_chunk:
 *  Reads an APP0/APP1 (JFIF/EXIF descriptor) chunk from the input stream.
 */
static int
read_appn_chunk(int n, int flags)
{
	char header[6] = { 0 };
	char *header_id;
	int i;
	
	if (n == CHUNK_APP0)
		header_id = "JFIF";
	else
		header_id = "Exif";
	
	_jpeg_open_chunk();
	if ((n == CHUNK_APP1) || (!(flags & APP0_DEFINED))) {
		for (i = 0; i < 5; i++) {
			if ((header[i] = _jpeg_getc()) != header_id[i]) {
				TRACE("Bad %s header, found %s", (n == CHUNK_APP0) ? "JFIF" : "EXIF", header);
				_jpeg_close_chunk();
				jpgalleg_error = JPG_ERROR_NOT_JPEG;
				return -1;
			}
		}
		if (n == CHUNK_APP0) {
			/* Only JFIF version 1.x is supported */
			if (_jpeg_getc() != 1) {
				TRACE("Not a JFIF version 1.x file");
				_jpeg_close_chunk();
				return -1;
			}
		}
	}
	_jpeg_close_chunk();
	return 0;
}


/* read_dri_chunk:
 *  Reads a DRI (Define Restart Interval) chunk from the input stream.
 */
static int
read_dri_chunk(void)
{
	_jpeg_open_chunk();
	restart_interval = _jpeg_getw();
	_jpeg_close_chunk();
	return 0;
}


/* get_bits:
 *  Reads a string of bits from the input stream.
 */
static int
get_bits(int num_bits)
{
	int result = 0;
	
	while (_jpeg_io.current_bit < num_bits) {
		result = (result << _jpeg_io.current_bit) | (*_jpeg_io.buffer & ((1 << _jpeg_io.current_bit) - 1));
		num_bits -= _jpeg_io.current_bit;
		_jpeg_io.current_bit = 8;
		if (*_jpeg_io.buffer == 0xff)
			_jpeg_io.buffer++;
		if (_jpeg_io.buffer >= _jpeg_io.buffer_end) {
			TRACE("Tried to read memory past buffer size");
			jpgalleg_error = JPG_ERROR_INPUT_BUFFER_TOO_SMALL;
			return 0x80000000;
		}
		_jpeg_io.buffer++;
	}
	result = (result << num_bits) | ((*_jpeg_io.buffer >> (_jpeg_io.current_bit - num_bits)) & ((1 << num_bits) - 1));
	_jpeg_io.current_bit -= num_bits;
	
	return result;
}


/* get_value:
 *  Reads a string of bits from the input stream and returns a properly signed
 *  number given the category.
 */
static INLINE int
get_value(int category)
{
	int result = get_bits(category);
	if ((result >= (1 << (category - 1))) || (result < 0))
		return result;
	else
		return result - ((1 << category) - 1);
}


/* huffman_decode:
 *  Fetches bits from the input stream until a valid huffman code is found,
 *  then returns the value associated with that code.
 */
static int
huffman_decode(HUFFMAN_TABLE *table)
{
	HUFFMAN_ENTRY *entry, **entry_lut;
	int i, value;
	unsigned char *p = _jpeg_io.buffer;
	
	value = (*p & ((1 << _jpeg_io.current_bit) - 1)) << (16 - _jpeg_io.current_bit);
	if (*p++ == 0xff) p++;
	if (p < _jpeg_io.buffer_end) {
		value |= *p << (8 - _jpeg_io.current_bit);
		if (*p++ == 0xff) p++;
		if (p < _jpeg_io.buffer_end)
			value |= *p >> _jpeg_io.current_bit;
	}
	
	entry_lut = table->entry_of_length;
	for (i = 15; i >= 0; i--) {
		entry = &((*entry_lut)[value >> i]);
		if (entry->bits_length == 16 - i) {
			_jpeg_io.current_bit -= 16 - i;
			while (_jpeg_io.current_bit <= 0) {
				_jpeg_io.current_bit += 8;
				if (*_jpeg_io.buffer == 0xff)
					_jpeg_io.buffer++;
				_jpeg_io.buffer++;
			}
			return entry->value;
		}
		entry_lut++;
	}
	return -1;
}


/* decode_baseline_block:
 *  Decodes an 8x8 basic block of coefficients of given type (luminance or
 *  chrominance) from the input stream. Used for baseline decoding.
 */
static int
decode_baseline_block(short *block, int type, int *old_dc)
{
	HUFFMAN_TABLE *dc_table, *ac_table;
	short *quant_table;
	int data, i, index;
	int num_zeroes, category;
	short workspace[130];
	short pre_idct_block[80];
	short ordered_pre_idct_block[64];
	
	if (type == LUMINANCE) {
		dc_table = dc_luminance_table;
		ac_table = ac_luminance_table;
		quant_table = luminance_quantization_table;
	}
	else {
		dc_table = dc_chrominance_table;
		ac_table = ac_chrominance_table;
		quant_table = chrominance_quantization_table;
	}
	
	data = huffman_decode(dc_table);
	if (data < 0) {
		TRACE("Bad dc data");
		jpgalleg_error = JPG_ERROR_BAD_IMAGE;
		return -1;
	}
	if ((data = get_value(data & 0xf)) == (int)0x80000000)
		return -1;
	*old_dc += data;
	pre_idct_block[0] = *old_dc;
	
	index = 1;
	do {
		data = huffman_decode(ac_table);
		if (data < 0) {
			/* Bad block */
			TRACE("Bad ac data");
			jpgalleg_error = JPG_ERROR_BAD_IMAGE;
			return -1;
		}
		num_zeroes = data >> 4;
		category = data & 0xf;
		if (category != 0) {
			/* Normal zero run length coding */
			for (; num_zeroes; num_zeroes--)
				pre_idct_block[index++] = 0;
			if ((data = get_value(category)) == (int)0x80000000)
				return -1;
			pre_idct_block[index++] = data;
		}
		else {
			if (num_zeroes == 0) {
				/* End of block */
				while (index < 64)
					pre_idct_block[index++] = 0;
				break;
			}
			else if (num_zeroes == 15) {
				/* 16 zeroes special case */
				for (i = 16; i; i--)
					pre_idct_block[index++] = 0;
			}
			else {
				TRACE("Bad ac data");
				jpgalleg_error = JPG_ERROR_BAD_IMAGE;
				return -1;
			}
		}
	} while (index < 64);
	
	zigzag_reorder(pre_idct_block, ordered_pre_idct_block);
	
	idct(ordered_pre_idct_block, block, quant_table, workspace);
	
	return 0;
}


/* decode_progressive_block
 *  Decodes some coefficients for an 8x8 block from the input stream. Used in
 *  progressive mode decoding.
 */
static int
decode_progressive_block(intptr_t addr, int type, int *old_dc)
{
	HUFFMAN_TABLE *dc_table, *ac_table;
	short *block = (short *)addr;
	int data, index, value;
	int num_zeroes, category;
	int p_bit, n_bit;
	
	if (type == LUMINANCE) {
		dc_table = dc_luminance_table;
		ac_table = ac_luminance_table;
	}
	else {
		dc_table = dc_chrominance_table;
		ac_table = ac_chrominance_table;
	}
	
	if (spectrum_start == 0) {
		/* DC scan */
		if (successive_high == 0) {
			/* First DC scan */
			data = huffman_decode(dc_table);
			if (data < 0) {
				TRACE("Bad dc data");
				jpgalleg_error = JPG_ERROR_BAD_IMAGE;
				return -1;
			}
			if ((data = get_value(data & 0xf)) == (int)0x80000000)
				return -1;
			*old_dc += data;
			block[0] = *old_dc << successive_low;
		}
		else {
			/* DC successive approximation */
			if ((data = _jpeg_get_bit()) < 0) {
				TRACE("Failed to get bit from input stream");
				jpgalleg_error = JPG_ERROR_BAD_IMAGE;
				return -1;
			}
			if (data)
				block[0] |= (1 << successive_low);
		}
	}
	else {
		/* AC scan */
		if (successive_high == 0) {
			/* First AC scan */
			if (skip_count) {
				skip_count--;
				return 0;
			}
			index = spectrum_start;
			do {
				data = huffman_decode(ac_table);
				if (data < 0) {
					TRACE("Bad ac data (first scan)");
					jpgalleg_error = JPG_ERROR_BAD_IMAGE;
					return -1;
				}
				num_zeroes = data >> 4;
				category = data & 0xf;
				if (category == 0) {
					if (num_zeroes == 15)
						index += 16;
					else {
						index++;
						skip_count = 0;
						if (num_zeroes) {
							value = get_bits(num_zeroes);
							if (value < 0) {
								TRACE("Failed to get bit from input stream");
								jpgalleg_error = JPG_ERROR_BAD_IMAGE;
								return -1;
							}
							skip_count = (1 << num_zeroes) + value - 1;
						}
						break;
					}
				}
				else {
					index += num_zeroes;
					if ((data = get_value(category)) == (int)0x80000000)
						return -1;
					block[index++] = data << successive_low;
				}
			} while (index <= spectrum_end);
		}
		else {
			/* AC successive approximation */
			index = spectrum_start;
			p_bit = 1 << successive_low;
			n_bit = (-1) << successive_low;
			if (skip_count == 0) {
				do {
					data = huffman_decode(ac_table);
					if (data < 0) {
						TRACE("Bad ac data");
						jpgalleg_error = JPG_ERROR_BAD_IMAGE;
						return -1;
					}
					num_zeroes = data >> 4;
					category = data & 0xf;
					if (category == 0) {
						if (num_zeroes < 15) {
							skip_count = 1 << num_zeroes;
							if (num_zeroes) {
								value = get_bits(num_zeroes);
								if (value < 0) {
									TRACE("Failed to get bit from input stream");
									jpgalleg_error = JPG_ERROR_BAD_IMAGE;
									return -1;
								}
								skip_count += value;
							}
							break;
						}
					}
					else if (category == 1) {
						if ((data = _jpeg_get_bit()) < 0) {
							TRACE("Failed to get bit from input stream");
							jpgalleg_error = JPG_ERROR_BAD_IMAGE;
							return -1;
						}
						if (data)
							category = p_bit;
						else
							category = n_bit;
					}
					else {
						TRACE("Unexpected ac value category");
						jpgalleg_error = JPG_ERROR_BAD_IMAGE;
						return -1;
					}
					do {
						if (block[index]) {
							if ((data = _jpeg_get_bit()) < 0) {
								TRACE("Failed to get bit from input stream");
								jpgalleg_error = JPG_ERROR_BAD_IMAGE;
								return -1;
							}
							if ((data) && (!(block[index] & p_bit))) {
								if (block[index] >= 0)
									block[index] += p_bit;
								else
									block[index] += n_bit;
							}
						}
						else {
							num_zeroes--;
							if (num_zeroes < 0)
								break;
						}
						index++;
					} while (index <= spectrum_end);
					if ((category) && (index < 64))
						block[index] = category;
					index++;
				} while (index <= spectrum_end);
			}
			if (skip_count > 0) {
				while (index <= spectrum_end) {
					if (block[index]) {
						if ((data = _jpeg_get_bit()) < 0) {
							TRACE("Failed to get bit from input stream");
							jpgalleg_error = JPG_ERROR_BAD_IMAGE;
							return -1;
						}
						if ((data) && (!(block[index] & p_bit))) {
							if (block[index] >= 0)
								block[index] += p_bit;
							else
								block[index] += n_bit;
						}
					}
					index++;
				}
				skip_count--;
			}
		}
	}
	
	return 0;
}


/* _jpeg_c_ycbcr2rgb:
 *  C version of the YCbCr -> RGB color conversion routine. Converts 2 pixels
 *  at a time.
 */
static void
_jpeg_c_ycbcr2rgb(intptr_t addr, int y1, int cb1, int cr1, int y2, int cb2, int cr2, int y3, int cb3, int cr3, int y4, int cb4, int cr4)
{
	int r, g, b;
	unsigned int *ptr = (unsigned int *)addr, temp, p0, p1, p2;

#ifdef ALLEGRO_LITTLE_ENDIAN
	r = MID(0, ((y1 << 8)                       + (359 * (cr1 - 128))) >> 8, 255);
	g = MID(0, ((y1 << 8) -  (88 * (cb1 - 128)) - (183 * (cr1 - 128))) >> 8, 255);
	b = MID(0, ((y1 << 8) + (453 * (cb1 - 128))                      ) >> 8, 255);
	p0 = makecol24(r, g, b);
	r = MID(0, ((y2 << 8)                       + (359 * (cr2 - 128))) >> 8, 255);
	g = MID(0, ((y2 << 8) -  (88 * (cb2 - 128)) - (183 * (cr2 - 128))) >> 8, 255);
	b = MID(0, ((y2 << 8) + (453 * (cb2 - 128))                      ) >> 8, 255);
	temp = makecol24(r, g, b);
	p0 |= (temp << 24);
	p1 = temp >> 8;
	r = MID(0, ((y3 << 8)                       + (359 * (cr3 - 128))) >> 8, 255);
	g = MID(0, ((y3 << 8) -  (88 * (cb3 - 128)) - (183 * (cr3 - 128))) >> 8, 255);
	b = MID(0, ((y3 << 8) + (453 * (cb3 - 128))                      ) >> 8, 255);
	temp = makecol24(r, g, b);
	p1 |= temp << 16;
	p2 = temp >> 16;
	r = MID(0, ((y4 << 8)                       + (359 * (cr4 - 128))) >> 8, 255);
	g = MID(0, ((y4 << 8) -  (88 * (cb4 - 128)) - (183 * (cr4 - 128))) >> 8, 255);
	b = MID(0, ((y4 << 8) + (453 * (cb4 - 128))                      ) >> 8, 255);
	p2 |= makecol24(r, g, b) << 8;
#else
	r = MID(0, ((y1 << 8)                       + (359 * (cr1 - 128))) >> 8, 255);
	g = MID(0, ((y1 << 8) -  (88 * (cb1 - 128)) - (183 * (cr1 - 128))) >> 8, 255);
	b = MID(0, ((y1 << 8) + (453 * (cb1 - 128))                      ) >> 8, 255);
	p0 = makecol24(r, g, b) << 8;
	r = MID(0, ((y2 << 8)                       + (359 * (cr2 - 128))) >> 8, 255);
	g = MID(0, ((y2 << 8) -  (88 * (cb2 - 128)) - (183 * (cr2 - 128))) >> 8, 255);
	b = MID(0, ((y2 << 8) + (453 * (cb2 - 128))                      ) >> 8, 255);
	temp = makecol24(r, g, b);
	p0 |= (temp >> 16);
	p1 = temp << 16;
	r = MID(0, ((y3 << 8)                       + (359 * (cr3 - 128))) >> 8, 255);
	g = MID(0, ((y3 << 8) -  (88 * (cb3 - 128)) - (183 * (cr3 - 128))) >> 8, 255);
	b = MID(0, ((y3 << 8) + (453 * (cb3 - 128))                      ) >> 8, 255);
	temp = makecol24(r, g, b);
	p1 |= temp >> 8;
	p2 = temp << 24;
	r = MID(0, ((y4 << 8)                       + (359 * (cr4 - 128))) >> 8, 255);
	g = MID(0, ((y4 << 8) -  (88 * (cb4 - 128)) - (183 * (cr4 - 128))) >> 8, 255);
	b = MID(0, ((y4 << 8) + (453 * (cb4 - 128))                      ) >> 8, 255);
	p2 |= makecol24(r, g, b);
#endif
	ptr[0] = p0;
	ptr[1] = p1;
	ptr[2] = p2;
}


/* plot_444:
 *  Plots an 8x8 MCU block for 444 mode. Also used to plot greyscale MCUs.
 */
static void
plot_444(intptr_t addr, int pitch, short *y1, short *y2, short *y3, short *y4, short *cb, short *cr)
{
	int x, y;
	short *y1_ptr = y1, *cb_ptr = cb, *cr_ptr = cr, v;
	
	(void)y2;
	(void)y3;
	(void)y4;
	
	if (jpeg_components == 1) {
		for (y = 0; y < 8; y++) {
			for (x = 0; x < 8; x++) {
				v = *y1_ptr++;
				*(unsigned char *)addr = MID(0, v, 255);
				addr++;
			}
			addr += (pitch - 8);
		}
	}
	else {
		for (y = 0; y < 8; y++) {
			for (x = 0; x < 8; x += 4) {
				ycbcr2rgb(addr, *y1_ptr, *cb_ptr, *cr_ptr, *(y1_ptr + 1), *(cb_ptr + 1), *(cr_ptr + 1), *(y1_ptr + 2), *(cb_ptr + 2), *(cr_ptr + 2), *(y1_ptr + 3), *(cb_ptr + 3), *(cr_ptr + 3));
				y1_ptr += 4;
				cb_ptr += 4;
				cr_ptr += 4;
				addr += 12;
			}
			addr += (pitch - 24);
		}
	}
}


/* plot_422_h:
 *  Plots a 16x8 MCU block for 422 mode.
 */
static void
plot_422_h(intptr_t addr, int pitch, short *y1, short *y2, short *y3, short *y4, short *cb, short *cr)
{
	int x, y;
	short *y1_ptr = y1, *y2_ptr = y2, *cb_ptr = cb, *cr_ptr = cr;
	
	(void)y3;
	(void)y4;
	
	for (y = 0; y < 8; y++) {
		for (x = 0; x < 8; x += 4) {
			ycbcr2rgb(addr, *y1_ptr, *cb_ptr, *cr_ptr, *(y1_ptr + 1), *cb_ptr, *cr_ptr, *(y1_ptr + 2), *(cb_ptr + 1), *(cr_ptr + 1), *(y1_ptr + 3), *(cb_ptr + 1), *(cr_ptr + 1));
			ycbcr2rgb(addr + 24, *y2_ptr, *(cb_ptr + 4), *(cr_ptr + 4), *(y2_ptr + 1), *(cb_ptr + 4), *(cr_ptr + 4), *(y2_ptr + 2), *(cb_ptr + 5), *(cr_ptr + 5), *(y2_ptr + 3), *(cb_ptr + 5), *(cr_ptr + 5));
			y1_ptr += 4;
			y2_ptr += 4;
			cb_ptr += 2;
			cr_ptr += 2;
			addr += 12;
		}
		cb_ptr += 4;
		cr_ptr += 4;
		addr += (pitch - 24);
	}
}


/* plot_422_v:
 *  Plots a 8x16 MCU block for 422 mode.
 */
static void
plot_422_v(intptr_t addr, int pitch, short *y1, short *y2, short *y3, short *y4, short *cb, short *cr)
{
	int x, y, d;
	short *y1_ptr = y1, *y2_ptr = y2, *cb_ptr = cb, *cr_ptr = cr;
	
	(void)y3;
	(void)y4;
	
	for (y = 0; y < 8; y++) {
		for (x = 0; x < 8; x += 4) {
			ycbcr2rgb(addr, *y1_ptr, *cb_ptr, *cr_ptr, *(y1_ptr + 1), *(cb_ptr + 1), *(cr_ptr + 1), *(y1_ptr + 2), *(cb_ptr + 2), *(cr_ptr + 2), *(y1_ptr + 3), *(cb_ptr + 3), *(cr_ptr + 3));
			ycbcr2rgb(addr + (pitch * 8), *y2_ptr, *(cb_ptr + 32), *(cr_ptr + 32), *(y2_ptr + 1), *(cb_ptr + 33), *(cr_ptr + 33), *(y2_ptr + 2), *(cb_ptr + 34), *(cr_ptr + 34), *(y2_ptr + 3), *(cb_ptr + 35), *(cr_ptr + 35));
			y1_ptr += 4;
			y2_ptr += 4;
			cb_ptr += 4;
			cr_ptr += 4;
			addr += 12;
		}
		d = (!(y & 1)) * 8;
		cb_ptr -= d;
		cr_ptr -= d;
		addr += (pitch - 24);
	}
}


/* plot_411:
 *  Plots a 16x16 MCU block for 411 mode.
 */
static void
plot_411(intptr_t addr, int pitch, short *y1, short *y2, short *y3, short *y4, short *cb, short *cr)
{
	int x, y, d;
	short *y1_ptr = y1, *y2_ptr = y2, *y3_ptr = y3, *y4_ptr = y4, *cb_ptr = cb, *cr_ptr = cr;
	
	for (y = 0; y < 8; y++) {
		for (x = 0; x < 8; x += 4) {
			ycbcr2rgb(addr, *y1_ptr, *cb_ptr, *cr_ptr, *(y1_ptr + 1), *cb_ptr, *cr_ptr, *(y1_ptr + 2), *(cb_ptr + 1), *(cr_ptr + 1), *(y1_ptr + 3), *(cb_ptr + 1), *(cr_ptr + 1));
			ycbcr2rgb(addr + 24, *y2_ptr, *(cb_ptr + 4), *(cr_ptr + 4), *(y2_ptr + 1), *(cb_ptr + 4), *(cr_ptr + 4), *(y2_ptr + 2), *(cb_ptr + 5), *(cr_ptr + 5), *(y2_ptr + 3), *(cb_ptr + 5), *(cr_ptr + 5));
			ycbcr2rgb(addr + (pitch * 8), *y3_ptr, *(cb_ptr + 32), *(cr_ptr + 32), *(y3_ptr + 1), *(cb_ptr + 32), *(cr_ptr + 32), *(y3_ptr + 2), *(cb_ptr + 33), *(cr_ptr + 33), *(y3_ptr + 3), *(cb_ptr + 33), *(cr_ptr + 33));
			ycbcr2rgb(addr + (pitch * 8) + 24, *y4_ptr, *(cb_ptr + 36), *(cr_ptr + 36), *(y4_ptr + 1), *(cb_ptr + 36), *(cr_ptr + 36), *(y4_ptr + 2), *(cb_ptr + 37), *(cr_ptr + 37), *(y4_ptr + 3), *(cb_ptr + 37), *(cr_ptr + 37));
			y1_ptr += 4;
			y2_ptr += 4;
			y3_ptr += 4;
			y4_ptr += 4;
			cb_ptr += 2;
			cr_ptr += 2;
			addr += 12;
		}
		d = ((y & 1) * 8) - 4;
		cb_ptr += d;
		cr_ptr += d;
		addr += (pitch - 24);
	}
}


#ifdef DEBUG
static void
dump_chunk(char *msg, int length)
{
	char buffer[65536];
	int i;
	
	for (i = 0; (i < length) && (!_jpeg_eoc()); i++)
		buffer[i] = _jpeg_getc();
	buffer[i] = '\0';
	
	TRACE("%s%s", msg, buffer);
}
#endif


/* do_decode:
 *  Main decoding function.
 */
static BITMAP *
do_decode(RGB *pal, void (*callback)(int))
{
	const int x_ofs[4] = { 0, 1, 0, 1 }, y_ofs[4] = { 0, 0, 1, 1 };
	short coefs_buffer[384], coefs[64], *coefs_ptr, *temp_ptr;
	short *y1, *y2, *y3, *y4, *cb, *cr;
	short workspace[130];
	intptr_t addr;
	int pitch, i, j;
	int block_x, block_y, block_max_x, block_max_y;
	int blocks_per_row[3];
	int blocks_in_mcu, block_component[6];
	short *block_ptr[6];
	int block_x_ofs[6], block_y_ofs[6];
	int mcu_w, mcu_h, component_w[3], component_h[3], c;
	int old_dc[3] = { 0, 0, 0 };
	BITMAP *bmp;
	int data, flags = 0;
	int restart_count;
	int depth;
	
	jpgalleg_error = JPG_ERROR_NONE;
	
	TRACE("############### Decode start ###############");
	
#ifdef JPGALLEG_MMX
	if (cpu_capabilities & CPU_MMX) {
		idct = _jpeg_mmx_idct;
		if (_rgb_r_shift_24 == 0)
			ycbcr2rgb = _jpeg_mmx_ycbcr2rgb;
		else if (_rgb_r_shift_24 == 16)
			ycbcr2rgb = _jpeg_mmx_ycbcr2bgr;
		else
			ycbcr2rgb = _jpeg_c_ycbcr2rgb;
		TRACE("Using MMX...");
	}
	else {
#endif
		idct = _jpeg_c_idct;
		ycbcr2rgb = _jpeg_c_ycbcr2rgb;
#ifdef JPGALLEG_MMX
	}
#endif

	memset(_jpeg_huffman_dc_table, 0, 4 * sizeof(HUFFMAN_TABLE));
	memset(_jpeg_huffman_ac_table, 0, 4 * sizeof(HUFFMAN_TABLE));
	y1 = coefs_buffer;
	y2 = coefs_buffer + 64;
	y3 = coefs_buffer + 128;
	y4 = coefs_buffer + 192;
	cb = coefs_buffer + 256;
	cr = coefs_buffer + 320;
	
	_jpeg_io.current_bit = 8;
	
	if (_jpeg_getw() != CHUNK_SOI) {
		TRACE("SOI chunk not found");
		jpgalleg_error = JPG_ERROR_NOT_JPEG;
		return NULL;
	}
	
	/* Examine header */
	do {
		data = _jpeg_getc();
		if (data < 0)
			return NULL;
		else if (data == 0xff) {
			while ((data = _jpeg_getc()) == 0xff)
				;
			switch (data) {
				case -1:
					return NULL;
				case 0:
					break;
				case CHUNK_SOF2:
					flags |= IS_PROGRESSIVE;
					/* fallthrough */
				case CHUNK_SOF0:
				case CHUNK_SOF1:
					TRACE("SOFx chunk found");
					if (read_sof0_chunk())
						return NULL;
					flags |= SOF0_DEFINED;
					break;
				case CHUNK_SOF3:
				case CHUNK_SOF5:
				case CHUNK_SOF6:
				case CHUNK_SOF7:
				case CHUNK_SOF9:
				case CHUNK_SOF10:
				case CHUNK_SOF11:
				case CHUNK_SOF13:
				case CHUNK_SOF14:
				case CHUNK_SOF15:
					TRACE("Unsupported encoding chunk (0xFF%X)", data);
					jpgalleg_error = JPG_ERROR_UNSUPPORTED_ENCODING;
					return NULL;
				case CHUNK_DHT:
					TRACE("DHT chunk found");
					if (read_dht_chunk())
						return NULL;
					flags |= DHT_DEFINED;
					break;
				case CHUNK_SOS:
					TRACE("SOS chunk found");
					if (read_sos_chunk())
						return NULL;
					flags |= SOS_DEFINED;
					break;
				case CHUNK_DQT:
					TRACE("DQT chunk found");
					if (read_dqt_chunk())
						return NULL;
					flags |= DQT_DEFINED;
					break;
				case CHUNK_APP0:
					TRACE("APP0 chunk found");
					if (read_appn_chunk(data, flags))
						return NULL;
					flags |= APP0_DEFINED;
					break;
				case CHUNK_APP1:
					TRACE("APP1 chunk found");
					if (!(flags & APP1_DEFINED)) {
						if (read_appn_chunk(data, flags))
							return NULL;
						flags |= APP1_DEFINED;
					}
					else {
						_jpeg_open_chunk();
						_jpeg_close_chunk();
					}
					break;
				case CHUNK_APP2:
				case CHUNK_APP3:
				case CHUNK_APP4:
				case CHUNK_APP5:
				case CHUNK_APP6:
				case CHUNK_APP7:
				case CHUNK_APP8:
				case CHUNK_APP9:
				case CHUNK_APP10:
				case CHUNK_APP11:
				case CHUNK_APP12:
				case CHUNK_APP13:
				case CHUNK_APP14:
				case CHUNK_APP15:
					TRACE("APP%d chunk found, skipping", data - CHUNK_APP0);
					_jpeg_open_chunk();
#ifdef DEBUG
					dump_chunk("First 30 bytes of chunk: ", 30);
#endif
					_jpeg_close_chunk();
					flags |= APP0_DEFINED;
					break;
					
				case CHUNK_DRI:
					TRACE("DRI chunk found");
					if (read_dri_chunk())
						return NULL;
					flags |= DRI_DEFINED;
					break;
				case CHUNK_JPG:
				case CHUNK_TEM:
				case CHUNK_RST0:
				case CHUNK_RST1:
				case CHUNK_RST2:
				case CHUNK_RST3:
				case CHUNK_RST4:
				case CHUNK_RST5:
				case CHUNK_RST6:
				case CHUNK_RST7:
					TRACE("Unexpected chunk found in header (0xFF%X)", data);
					jpgalleg_error = JPG_ERROR_BAD_IMAGE;
					return NULL;
				case CHUNK_COM:
					_jpeg_open_chunk();
#ifdef DEBUG
					dump_chunk("COM chunk found; comment: ", 65536);
#endif
					_jpeg_close_chunk();
					break;
				default:
					TRACE("Unknown chunk found in header (0xFF%X), skipping", data);
					_jpeg_open_chunk();
#ifdef DEBUG
					dump_chunk("First 30 bytes of chunk: ", 30);
#endif
					_jpeg_close_chunk();
					break;
			}
		}
	} while (((flags & JFIF_OK) != JFIF_OK) && ((flags & EXIF_OK) != EXIF_OK));
	
	/* Deal with bogus restart interval */
	if (restart_interval <= 0)
		flags &= ~DRI_DEFINED;
	
	bmp = create_bitmap_ex((jpeg_components == 1) ? 8 : 24, (jpeg_w + 15) & ~0xf, (jpeg_h + 15) & ~0xf);
	if (!bmp) {
		TRACE("Out of memory");
		return NULL;
	}
	pitch = (int)(bmp->line[1] - bmp->line[0]);
	
	block_x = block_y = 0;
	restart_count = 0;
	memset(data_buffer, 0, 3 * sizeof(DATA_BUFFER *));
	
	progress_cb = callback;
	progress_counter = 0;
	
	if (!(flags & IS_PROGRESSIVE)) {
		/* Baseline decoding */
		TRACE("Starting baseline decoding");
		if (jpeg_components == 1)
			/* force 444 mode for grayscale images */
			sampling = h_sampling = v_sampling = 1;
		blocks_in_mcu = 0;
		coefs_ptr = coefs_buffer;
		for (i = 0; i < sampling; i++) {
			block_component[blocks_in_mcu] = 0;
			block_ptr[blocks_in_mcu] = coefs_ptr;
			coefs_ptr += 64;
			blocks_in_mcu++;
		}
		for (i = 1; i < jpeg_components; i++) {
			block_component[blocks_in_mcu] = i;
			block_ptr[blocks_in_mcu] = coefs_ptr;
			coefs_ptr += 64;
			blocks_in_mcu++;
		}
		mcu_w = h_sampling * 8;
		mcu_h = v_sampling * 8;
		plot = plot_411;
		if (sampling < 4) {
			plot = plot_422_v;
			if (h_sampling == 2)
				plot = plot_422_h;
			cb -= 128;
			cr -= 128;
			if (sampling < 2) {
				plot = plot_444;
				cb -= 64;
				cr -= 64;
			}
		}
		
		progress_total = (bmp->w / mcu_w) * (bmp->h / mcu_h);
		
		TRACE("%dx%d %s image, %s mode", jpeg_w, jpeg_h, jpeg_components == 1 ? "greyscale" : "color", plot == plot_444 ? "444" : (((plot == plot_422_h) || (plot == plot_422_v)) ? "422" : "411"));
		/* Start decoding! */
		do {
			for (i = 0; i < blocks_in_mcu; i++) {
				if (decode_baseline_block(block_ptr[i], (block_component[i] == 0) ? LUMINANCE : CHROMINANCE, &old_dc[block_component[i]]))
					goto exit_error;
			}
			addr = (intptr_t)bmp->line[block_y] + (block_x * (jpeg_components == 1 ? 1 : 3));
			plot(addr, pitch, y1, y2, y3, y4, cb, cr);
			block_x += mcu_w;
			if (block_x >= jpeg_w) {
				block_x = 0;
				block_y += mcu_h;
			}
			restart_count++;
			if ((flags & DRI_DEFINED) && (restart_count >= restart_interval)) {
				data = _jpeg_getw();
				if (data == CHUNK_EOI)
					break;
				if ((data < CHUNK_RST0) || (data > CHUNK_RST7)) {
					TRACE("Expected RSTx chunk not found, found 0x%X instead", data);
					jpgalleg_error = JPG_ERROR_BAD_IMAGE;
					goto exit_error;
				}
				memset(old_dc, 0, 3 * sizeof(int));
				restart_count = 0;
			}
			if (progress_cb)
				progress_cb((progress_counter * 100) / progress_total);
			progress_counter++;
		} while (block_y < jpeg_h);
	}
	else {
		/* Progressive decoding */
		TRACE("Starting progressive decoding");
		blocks_per_row[0] = bmp->w / 8;
		data_buffer[0] = (DATA_BUFFER *)calloc(1, sizeof(DATA_BUFFER) * (bmp->w / 8) * (bmp->h / 8));
		if (!data_buffer[0]) {
			TRACE("Out of memory");
			jpgalleg_error = JPG_ERROR_OUT_OF_MEMORY;
			goto exit_error;
		}
		for (i = 1; i < jpeg_components; i++) {
			blocks_per_row[i] = bmp->w / (h_sampling * 8);
			component_w[i] = component_h[i] = 1;
			data_buffer[i] = (DATA_BUFFER *)calloc(1, sizeof(DATA_BUFFER) * (bmp->w / 8) * (bmp->h / 8) / sampling);
			if (!data_buffer[i]) {
				TRACE("Out of memory");
				jpgalleg_error = JPG_ERROR_OUT_OF_MEMORY;
				goto exit_error;
			}
		}
		
		progress_total = (2 + (3 * jpeg_components)) * blocks_per_row[0] * (bmp->h / (v_sampling * 8));
		
		TRACE("%dx%d image, %s mode", jpeg_w, jpeg_h, sampling == 1 ? "444" : (sampling == 2 ? "422" : "411"));
		while (1) {
			/* Decode new scan */
			if (((spectrum_start > spectrum_end) || (spectrum_end > 63)) ||
					((spectrum_start == 0) && (spectrum_end != 0)) ||
					((successive_high != 0) && (successive_high != successive_low + 1))) {
				TRACE("Bad progressive scan parameters");
				jpgalleg_error = JPG_ERROR_BAD_IMAGE;
				goto exit_error;
			}
			restart_count = 0;
			block_x = block_y = 0;
			memset(old_dc, 0, 3 * sizeof(int));
			/* Setup MCU layout for this scan */
			blocks_in_mcu = 0;
			mcu_w = mcu_h = 8;
			for (i = 0; i < scan_components; i++) {
				switch (component[i]) {
					case 1:
						for (j = 0; j < sampling; j++) {
							block_component[blocks_in_mcu] = 0;
							block_x_ofs[blocks_in_mcu] = x_ofs[j];
							block_y_ofs[blocks_in_mcu] = y_ofs[j];
							blocks_in_mcu++;
						}
						if ((h_sampling == 1) && (v_sampling == 2)) {
							block_x_ofs[1] = x_ofs[2];
							block_y_ofs[1] = y_ofs[2];
						}
						break;
						
					case 2:
					case 3:
						block_component[blocks_in_mcu] = component[i] - 1;
						block_x_ofs[blocks_in_mcu] = 0;
						block_y_ofs[blocks_in_mcu] = 0;
						blocks_in_mcu++;
						mcu_w = MAX(mcu_w, h_sampling * 8);
						mcu_h = MAX(mcu_h, v_sampling * 8);
						break;
				}
			}
			block_max_x = (((jpeg_w + mcu_w - 1) & ~(mcu_w - 1)) / mcu_w);
			block_max_y = (((jpeg_h + mcu_h - 1) & ~(mcu_h - 1)) / mcu_h);
			/* Remove sampling dependency from luminance only scans */
			if ((scan_components == 1) && (block_component[0] == 0)) {
				blocks_in_mcu = 1;
				component_w[0] = component_h[0] = 1;
			}
			else {
				component_w[0] = h_sampling;
				component_h[0] = v_sampling;
			}
			TRACE("Starting new scan (%s%s%s, %dx%d MCU)", _jpeg_component_name[component[0] - 1],
				(scan_components > 1 ? _jpeg_component_name[component[1] - 1] : ""),
				(scan_components > 2 ? _jpeg_component_name[component[2] - 1] : ""), mcu_w, mcu_h);
			/* Start decoding! */
			do {
				restart_count++;
				if ((flags & DRI_DEFINED) && (restart_count >= restart_interval)) {
					data = _jpeg_getw();
					if ((data < CHUNK_RST0) || (data > CHUNK_RST7)) {
						TRACE("Expected RSTx chunk not found, found 0x%X instead", data);
						jpgalleg_error = JPG_ERROR_BAD_IMAGE;
						goto exit_error;
					}
					memset(old_dc, 0, 3 * sizeof(int));
					restart_count = skip_count = 0;
				}
				for (i = 0; i < blocks_in_mcu; i++) {
					c = block_component[i];
					addr = (intptr_t)(data_buffer[c][((block_y * component_h[c]) * blocks_per_row[c]) + (block_y_ofs[i] * blocks_per_row[c]) + (block_x * component_w[c]) + block_x_ofs[i]].data);
					if (decode_progressive_block(addr, (c == 0) ? LUMINANCE : CHROMINANCE, &old_dc[c]))
						goto exit_error;
				}
				block_x++;
				if (block_x >= block_max_x) {
					block_x = 0;
					block_y++;
				}
				if (progress_cb)
					progress_cb((progress_counter * 100) / progress_total);
				progress_counter++;
				if (progress_counter > progress_total)
					progress_total += (bmp->w / mcu_w) * (bmp->h / mcu_h);
			} while (block_y < block_max_y);
			/* Process inter-scan chunks */
			while (1) {
				while ((data = _jpeg_getc()) == 0xff)
					;
				if (data == CHUNK_SOS) {
					if (read_sos_chunk()) {
						jpgalleg_error = JPG_ERROR_BAD_IMAGE;
						goto exit_error;
					}
					break;
				}
				else if (data == CHUNK_DHT) {
					if (read_dht_chunk()) {
						jpgalleg_error = JPG_ERROR_BAD_IMAGE;
						goto exit_error;
					}
				}
				else if (data == CHUNK_DRI) {
					if (read_dri_chunk()) {
						jpgalleg_error = JPG_ERROR_BAD_IMAGE;
						goto exit_error;
					}
				}
				else if (data == (CHUNK_EOI & 0xff))
					goto eoi_found;
				else {
					TRACE("Unexpected inter-scan chunk found (0xFF%X)", data);
					jpgalleg_error = JPG_ERROR_BAD_IMAGE;
					goto exit_error;
				}
			}
		}
eoi_found:
		/* Apply idct and plot image */
		component_w[0] = h_sampling;
		component_h[0] = v_sampling;
		mcu_w = h_sampling * 8;
		mcu_h = v_sampling * 8;
		blocks_in_mcu = sampling;
		for (i = 0; i < sampling; i++) {
			block_component[i] = 0;
			block_x_ofs[i] = x_ofs[i];
			block_y_ofs[i] = y_ofs[i];
		}
		if ((h_sampling == 1) && (v_sampling == 2)) {
			block_x_ofs[1] = x_ofs[2];
			block_y_ofs[1] = y_ofs[2];
		}
		for (i = 1; i < jpeg_components; i++) {
			block_component[blocks_in_mcu] = i;
			block_x_ofs[blocks_in_mcu] = 0;
			block_y_ofs[blocks_in_mcu] = 0;
			blocks_in_mcu++;
		}
		plot = plot_411;
		if (sampling < 4) {
			plot = plot_422_v;
			if (h_sampling == 2)
				plot = plot_422_h;
			cb -= 128;
			cr -= 128;
			if (sampling < 2) {
				plot = plot_444;
				cb -= 64;
				cr -= 64;
			}
		}
		for (block_y = 0; block_y < bmp->h / mcu_h; block_y++) {
			for (block_x = 0; block_x < bmp->w / mcu_w; block_x++) {
				coefs_ptr = coefs_buffer;
				for (i = 0; i < blocks_in_mcu; i++) {
					c = block_component[i];
					temp_ptr = data_buffer[c][(block_y * blocks_per_row[c] * component_h[c]) + (blocks_per_row[c] * block_y_ofs[i]) + (block_x * component_w[c]) + block_x_ofs[i]].data;
					zigzag_reorder(temp_ptr, coefs);
					idct(coefs, coefs_ptr, (c == 0) ? luminance_quantization_table : chrominance_quantization_table, workspace);
					coefs_ptr += 64;
				}
				addr = (intptr_t)bmp->line[block_y * mcu_h] + (block_x * mcu_w * (jpeg_components == 1 ? 1 : 3));
				plot(addr, pitch, y1, y2, y3, y4, cb, cr);
			}
		}
	}

	/* Fixup image depth and size */
	if (jpeg_components == 1) {
		for (i = 0; i < 256; i++)
			pal[i].r = pal[i].g = pal[i].b = (i >> 2);
		depth = _color_load_depth(8, FALSE);
	}
	else
		depth = _color_load_depth(24, FALSE);
	if (((jpeg_components == 1) && (depth != 8)) || ((jpeg_components != 1) && (depth != 24)))
		bmp = _fixup_loaded_bitmap(bmp, pal, depth);
	if (depth != 8)
		generate_332_palette(pal);
	/* Hack to set size; image may be really slightly bigger than reported.
	 * We assume final user always to access data via line pointers and NEVER
	 * assume data is linearly stored in memory starting at bmp->dat...
	 */
	bmp->w = bmp->cr = jpeg_w;
	bmp->h = bmp->cb = jpeg_h;
	
exit_ok:
	for (i = 0; i < jpeg_components; i++) {
		if (data_buffer[i])
			free(data_buffer[i]);
	}
	for (i = 0; i < 4; i++) {
		free_huffman_table(&_jpeg_huffman_dc_table[i]);
		free_huffman_table(&_jpeg_huffman_ac_table[i]);
	}
	
	TRACE("################ Decode end ################");
	
	return bmp;

exit_error:
	destroy_bitmap(bmp);
	bmp = NULL;
	goto exit_ok;
}


/* load_jpg:
 *  Loads a JPG image from a file into a BITMAP, with no progress callback.
 */
BITMAP *
load_jpg(AL_CONST char *filename, RGB *palette)
{
	return load_jpg_ex(filename, palette, NULL);
}


/* load_jpg_ex:
 *  Loads a JPG image from a file into a BITMAP.
 */
BITMAP *
load_jpg_ex(AL_CONST char *filename, RGB *palette, void (*callback)(int progress))
{
	PACKFILE *f;
	BITMAP *bmp;
	PALETTE pal;
	uint64_t size;
	
	if (!palette)
		palette = pal;
	
	size = file_size_ex(filename);
	if (!size) {
		TRACE("File %s has zero size or does not exist", filename);
		jpgalleg_error = JPG_ERROR_READING_FILE;
		return NULL;
	}
	_jpeg_io.buffer = _jpeg_io.buffer_start = (unsigned char *)malloc(size);
	_jpeg_io.buffer_end = _jpeg_io.buffer_start + size;
	if (!_jpeg_io.buffer) {
		TRACE("Out of memory");
		jpgalleg_error = JPG_ERROR_OUT_OF_MEMORY;
		return NULL;
	}
	f = pack_fopen(filename, F_READ);
	if (!f) {
		TRACE("Cannot open %s for reading", filename);
		jpgalleg_error = JPG_ERROR_READING_FILE;
		free(_jpeg_io.buffer);
		return NULL;
	}
	pack_fread(_jpeg_io.buffer, size, f);
	pack_fclose(f);
	
	TRACE("Loading JPG from file %s", filename);
	
	bmp = do_decode(palette, callback);
	
	free(_jpeg_io.buffer_start);
	return bmp;
}


/* load_memory_jpg:
 *  Loads a JPG image from a memory buffer into a BITMAP, with no progress
 *  callback.
 */
BITMAP *
load_memory_jpg(void *buffer, int size, RGB *palette)
{
	return load_memory_jpg_ex(buffer, size, palette, NULL);
}


/* load_memory_jpg:
 *  Loads a JPG image from a memory buffer into a BITMAP.
 */
BITMAP *
load_memory_jpg_ex(void *buffer, int size, RGB *palette, void (*callback)(int progress))
{
	BITMAP *bmp;
	PALETTE pal;
	
	if (!palette)
		palette = pal;
	
	_jpeg_io.buffer = _jpeg_io.buffer_start = buffer;
	_jpeg_io.buffer_end = _jpeg_io.buffer_start + size;
	
	TRACE("Loading JPG from memory buffer at %p (size = %d)", buffer, size);
	
	bmp = do_decode(palette, callback);

	return bmp;
}
