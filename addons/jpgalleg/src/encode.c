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
 *      Encoder core module.
 *
 *      See the readme.txt file for instructions on using this package in your
 *      own programs.
 */


#include <internal.h>


/* Standard quantization tables for luminance and chrominance. Scaled version
 * of these are used by the encoder for a given quality.
 * These tables come from the IJG code, which takes them from the JPeg specs,
 * and are generic quantization tables that give good results on most images.
 */
static const unsigned char default_luminance_quant_table[64] = {
	16,  11,  10,  16,  24,  40,  51,  61,
	12,  12,  14,  19,  26,  58,  60,  55,
	14,  13,  16,  24,  40,  57,  69,  56,
	14,  17,  22,  29,  51,  87,  80,  62,
	18,  22,  37,  56,  68, 109, 103,  77,
	24,  35,  55,  64,  81, 104, 113,  92,
	49,  64,  78,  87, 103, 121, 120, 101,
	72,  92,  95,  98, 112, 100, 103,  99
};
static const unsigned char default_chrominance_quant_table[64] = {
	17,  18,  24,  47,  99,  99,  99,  99,
	18,  21,  26,  66,  99,  99,  99,  99,
	24,  26,  56,  99,  99,  99,  99,  99,
	47,  66,  99,  99,  99,  99,  99,  99,
	99,  99,  99,  99,  99,  99,  99,  99,
	99,  99,  99,  99,  99,  99,  99,  99,
	99,  99,  99,  99,  99,  99,  99,  99,
	99,  99,  99,  99,  99,  99,  99,  99
};

/* Standard huffman tables for luminance AC/DC and chrominance AC/DC.
 * These come from the IJG code, which takes them from the JPeg standard.
 */
static const unsigned char default_num_codes_dc_luminance[17] =
	{ 0, 0, 1, 5, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0 };
static const unsigned char default_val_dc_luminance[] =
	{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 };

static const unsigned char default_num_codes_dc_chrominance[17] =
	{ 0, 0, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0 };
static const unsigned char default_val_dc_chrominance[] =
	{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 };

static const unsigned char default_num_codes_ac_luminance[17] =
	{ 0, 0, 2, 1, 3, 3, 2, 4, 3, 5, 5, 4, 4, 0, 0, 1, 0x7d };
static const unsigned char default_val_ac_luminance[] = {
	0x01, 0x02, 0x03, 0x00, 0x04, 0x11, 0x05, 0x12,
	0x21, 0x31, 0x41, 0x06, 0x13, 0x51, 0x61, 0x07,
	0x22, 0x71, 0x14, 0x32, 0x81, 0x91, 0xa1, 0x08,
	0x23, 0x42, 0xb1, 0xc1, 0x15, 0x52, 0xd1, 0xf0,
	0x24, 0x33, 0x62, 0x72, 0x82, 0x09, 0x0a, 0x16,
	0x17, 0x18, 0x19, 0x1a, 0x25, 0x26, 0x27, 0x28,
	0x29, 0x2a, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39,
	0x3a, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49,
	0x4a, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59,
	0x5a, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69,
	0x6a, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79,
	0x7a, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89,
	0x8a, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98,
	0x99, 0x9a, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7,
	0xa8, 0xa9, 0xaa, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6,
	0xb7, 0xb8, 0xb9, 0xba, 0xc2, 0xc3, 0xc4, 0xc5,
	0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xd2, 0xd3, 0xd4,
	0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda, 0xe1, 0xe2,
	0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea,
	0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8,
	0xf9, 0xfa
};

static const unsigned char default_num_codes_ac_chrominance[17] =
	{ 0, 0, 2, 1, 2, 4, 4, 3, 4, 7, 5, 4, 4, 0, 1, 2, 0x77 };
static const unsigned char default_val_ac_chrominance[] = {
	0x00, 0x01, 0x02, 0x03, 0x11, 0x04, 0x05, 0x21,
	0x31, 0x06, 0x12, 0x41, 0x51, 0x07, 0x61, 0x71,
	0x13, 0x22, 0x32, 0x81, 0x08, 0x14, 0x42, 0x91,
	0xa1, 0xb1, 0xc1, 0x09, 0x23, 0x33, 0x52, 0xf0,
	0x15, 0x62, 0x72, 0xd1, 0x0a, 0x16, 0x24, 0x34,
	0xe1, 0x25, 0xf1, 0x17, 0x18, 0x19, 0x1a, 0x26,
	0x27, 0x28, 0x29, 0x2a, 0x35, 0x36, 0x37, 0x38,
	0x39, 0x3a, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48,
	0x49, 0x4a, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58,
	0x59, 0x5a, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68,
	0x69, 0x6a, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78,
	0x79, 0x7a, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,
	0x88, 0x89, 0x8a, 0x92, 0x93, 0x94, 0x95, 0x96,
	0x97, 0x98, 0x99, 0x9a, 0xa2, 0xa3, 0xa4, 0xa5,
	0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xb2, 0xb3, 0xb4,
	0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xc2, 0xc3,
	0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xd2,
	0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda,
	0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9,
	0xea, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8,
	0xf9, 0xfa
};


static unsigned char *num_codes_dc_luminance, *val_dc_luminance;
static unsigned char *num_codes_dc_chrominance, *val_dc_chrominance;
static unsigned char *num_codes_ac_luminance, *val_ac_luminance;
static unsigned char *num_codes_ac_chrominance, *val_ac_chrominance;
static int luminance_quant_table[64];
static int chrominance_quant_table[64];
static int current_pass, progress_counter, progress_total;
static int sampling, greyscale, mcu_w, mcu_h, pitch;
static BITMAP *fixed_bmp;
static void (*rgb2ycbcr)(intptr_t address, short *y1, short *cb1, short *cr1, short *y2, short *cb2, short *cr2);
static void (*progress_cb)(int percentage);



/* apply_fdct:
 *  Applies the forward discrete cosine transform to the given input block,
 *  in the form of a vector of 64 coefficients.
 *  This uses integer fixed point math and is based on code by the IJG.
 */
static void
apply_fdct(short *data)
{
	int tmp0, tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7;
	int tmp10, tmp11, tmp12, tmp13;
	int z1, z2, z3, z4, z5;
	short *dataptr = data;
	int i;
	
	for (i = 8; i; i--) {
		tmp0 = dataptr[0] + dataptr[7];
		tmp7 = dataptr[0] - dataptr[7];
		tmp1 = dataptr[1] + dataptr[6];
		tmp6 = dataptr[1] - dataptr[6];
		tmp2 = dataptr[2] + dataptr[5];
		tmp5 = dataptr[2] - dataptr[5];
		tmp3 = dataptr[3] + dataptr[4];
		tmp4 = dataptr[3] - dataptr[4];
		
		tmp10 = tmp0 + tmp3;
		tmp13 = tmp0 - tmp3;
		tmp11 = tmp1 + tmp2;
		tmp12 = tmp1 - tmp2;
		
		dataptr[0] = (tmp10 + tmp11) << 2;
		dataptr[4] = (tmp10 - tmp11) << 2;
		
		z1 = (tmp12 + tmp13) * FIX_0_541196100;
		dataptr[2] = (z1 + (tmp13 * FIX_0_765366865)) >> 11;
		dataptr[6] = (z1 + (tmp12 * -FIX_1_847759065)) >> 11;
		
		z1 = tmp4 + tmp7;
		z2 = tmp5 + tmp6;
		z3 = tmp4 + tmp6;
		z4 = tmp5 + tmp7;
		z5 = (z3 + z4) * FIX_1_175875602;
		
		tmp4 *= FIX_0_298631336;
		tmp5 *= FIX_2_053119869;
		tmp6 *= FIX_3_072711026;
		tmp7 *= FIX_1_501321110;
		z1 *= -FIX_0_899976223;
		z2 *= -FIX_2_562915447;
		z3 *= -FIX_1_961570560;
		z4 *= -FIX_0_390180644;
		
		z3 += z5;
		z4 += z5;
		
		dataptr[7] = (tmp4 + z1 + z3) >> 11;
		dataptr[5] = (tmp5 + z2 + z4) >> 11;
		dataptr[3] = (tmp6 + z2 + z3) >> 11;
		dataptr[1] = (tmp7 + z1 + z4) >> 11;
		
		dataptr += 8;
	}
	
	dataptr = data;
	for (i = 8; i; i--) {
		tmp0 = dataptr[0] + dataptr[56];
		tmp7 = dataptr[0] - dataptr[56];
		tmp1 = dataptr[8] + dataptr[48];
		tmp6 = dataptr[8] - dataptr[48];
		tmp2 = dataptr[16] + dataptr[40];
		tmp5 = dataptr[16] - dataptr[40];
		tmp3 = dataptr[24] + dataptr[32];
		tmp4 = dataptr[24] - dataptr[32];
		
		tmp10 = tmp0 + tmp3;
		tmp13 = tmp0 - tmp3;
		tmp11 = tmp1 + tmp2;
		tmp12 = tmp1 - tmp2;
		
		dataptr[0] = (tmp10 + tmp11) >> 2;
		dataptr[32] = (tmp10 - tmp11) >> 2;
		
		z1 = (tmp12 + tmp13) * FIX_0_541196100;
		dataptr[16] = (z1 + (tmp13 * FIX_0_765366865)) >> 15;
		dataptr[48] = (z1 + (tmp12 * -FIX_1_847759065)) >> 15;
		
		z1 = tmp4 + tmp7;
		z2 = tmp5 + tmp6;
		z3 = tmp4 + tmp6;
		z4 = tmp5 + tmp7;
		z5 = (z3 + z4) * FIX_1_175875602;
		
		tmp4 *= FIX_0_298631336;
		tmp5 *= FIX_2_053119869;
		tmp6 *= FIX_3_072711026;
		tmp7 *= FIX_1_501321110;
		z1 *= -FIX_0_899976223;
		z2 *= -FIX_2_562915447;
		z3 *= -FIX_1_961570560;
		z4 *= -FIX_0_390180644;
		
		z3 += z5;
		z4 += z5;
		dataptr[56] = (tmp4 + z1 + z3) >> 15;
		dataptr[40] = (tmp5 + z2 + z4) >> 15;
		dataptr[24] = (tmp6 + z2 + z3) >> 15;
		dataptr[8] = (tmp7 + z1 + z4) >> 15;
		
		dataptr++;
	}
}


/* zigzag_reorder:
 *  Reorders a vector of coefficients by the zigzag scan.
 */
static INLINE void
zigzag_reorder(short *input, short *output)
{
	int i;
	
	for (i = 0; i < 64; i++)
		output[_jpeg_zigzag_scan[i]] = input[i];
}


/* write_quantization_table:
 *  Computes a quantization table given a quality value and writes it to the
 *  output stream.
 */
static void
write_quantization_table(int *quant_table, const unsigned char *data, int quality)
{
	short temp[64], temp_table[64];
	double value, factor = QUALITY_FACTOR(quality);
	int i;

	for (i = 0; i < 64; i++) {
		if (quality == 100)
			value = (double)data[i] / 15.0;
		else
			value = (double)data[i] / factor;
		temp[i] = MID(1, (int)floor(value), 255);
	}
	zigzag_reorder(temp, temp_table);
	for (i = 0; i < 64; i++) {
		_jpeg_chunk_putc(temp_table[i]);
		quant_table[i] = (1 << 16) / (int)temp_table[i];
	}
}


/* setup_huffman_tree:
 *  Sets bit lengths and codes of each huffman tree node, and computes total
 *  tree depth. Also creates lists of nodes containing all leafs of the same
 *  level, from leftmost to rightmost leaf, if "list" is not NULL.
 */
static void
setup_huffman_tree(HUFFMAN_TREE *tree, HUFFMAN_NODE *node, int bits_length, HUFFMAN_NODE **list)
{
	HUFFMAN_NODE *tail;
	
	if (node->entry) {
		node->entry->bits_length = bits_length;
		if (list) {
			if (!list[bits_length]) {
				list[bits_length] = node;
			}
			else {
				tail = list[bits_length];
				while (tail->next) tail = tail->next;
				tail->next = node;
			}
			node->next = NULL;
		}
	}
	if (bits_length > tree->depth)
		tree->depth = bits_length;
	if (node->left) {
		setup_huffman_tree(tree, node->left, bits_length + 1, list);
		node->left->parent = node;
	}
	if (node->right) {
		setup_huffman_tree(tree, node->right, bits_length + 1, list);
		node->right->parent = node;
	}
}


/* find_leaf_at_level:
 *  Finds first huffman tree node holding a leaf at specified level.
 */
static HUFFMAN_NODE *
find_leaf_at_level(HUFFMAN_NODE *node, int level, int cur_level)
{
	HUFFMAN_NODE *result;
	
	if (node->entry) {
		if (cur_level == level)
			return node;
		return NULL;
	}
	result = find_leaf_at_level(node->left, level, cur_level + 1);
	if (result)
		return result;
	result = find_leaf_at_level(node->right, level, cur_level + 1);
	if (result)
		return result;
	return NULL;
}


/* destroy_huffman_tree:
 *  Frees memory used by specified huffman tree.
 */
static void
destroy_huffman_tree(HUFFMAN_NODE *node)
{
	if (!node)
		return;
	destroy_huffman_tree(node->left);
	destroy_huffman_tree(node->right);
	free(node);
}


/* compare_entries:
 *  qsort() comparision function to sort huffman entries by frequency.
 */
static int
compare_entries(const void *d1, const void *d2)
{
	HUFFMAN_ENTRY *e1 = (HUFFMAN_ENTRY *)d1;
	HUFFMAN_ENTRY *e2 = (HUFFMAN_ENTRY *)d2;
	
	if (e1->frequency < e2->frequency)
		return 1;
	else if (e1->frequency > e2->frequency)
		return -1;
	return 0;
}


/* build_huffman_table:
 *  Builds an optimized huffman table, given the frequency of each value.
 */
static int
build_huffman_table(HUFFMAN_TABLE *table, unsigned char *num_codes, unsigned char *value)
{
	HUFFMAN_TREE tree;
	HUFFMAN_NODE *node, *new_node, *src_node, *dest_node, *right, *left, *list[17];
	HUFFMAN_ENTRY *entry, *fake_entry = NULL;
	int i, level, max_frequency = 0;

	TRACE("Building new huffman tree");
	tree.head = tree.tail = NULL;
	tree.depth = 0;
	for (i = 0; i < 256; i++)
		max_frequency = MAX(max_frequency, table->entry[i].frequency);
	table->entry[256].frequency = max_frequency - 1;
	table->entry[256].value = -1;
	qsort(table->entry, 257, sizeof(HUFFMAN_ENTRY), compare_entries);
	for (i = 0; i < 257; i++) {
		entry = &table->entry[i];
		if (entry->value < 0)
			fake_entry = entry;
		if (entry->frequency > 0) {
			new_node = (HUFFMAN_NODE *)calloc(1, sizeof(HUFFMAN_NODE));
			new_node->entry = entry;
			new_node->frequency = entry->frequency;
			if (tree.tail) {
				tree.tail->next = new_node;
				new_node->prev = tree.tail;
				tree.tail = new_node;
			}
			else
				tree.head = tree.tail = new_node;
		}
	}
	while (tree.head != tree.tail) {
		new_node = (HUFFMAN_NODE *)calloc(1, sizeof(HUFFMAN_NODE));
		new_node->left = tree.tail;
		new_node->right = tree.tail->prev;
		new_node->frequency = new_node->left->frequency + new_node->right->frequency;
		tree.tail = tree.tail->prev->prev;
		if (tree.tail)
			tree.tail->next = NULL;
		else
			tree.head = NULL;
		for (node = tree.tail; node && (node->frequency < new_node->frequency); node = node->prev)
			;
		if (node) {
			new_node->prev = node;
			new_node->next = node->next;
			if (node->next)
				node->next->prev = new_node;
			else
				tree.tail = new_node;
			node->next = new_node;
		}
		else {
			new_node->next = tree.head;
			new_node->prev = NULL;
			if (tree.head)
				tree.head->prev = new_node;
			else
				tree.tail = new_node;
			tree.head = new_node;
		}
	}

	setup_huffman_tree(&tree, tree.head, 0, NULL);
	
	/*
	 *  Adjusts the huffman tree by removing two leaves from a level and
	 *  repositioning them at a lower level. Right leaf replaces the parent tree
	 *  node, left leaf is placed at the first convenient place at lower level.
	 *  This algorithm is used to reposition entries which appear to have a bits
	 *  length higher than 16, which is the maximum allowed by the JPG standard.
	 */
	while (tree.depth > 16) {
		TRACE("Tree depth > 16; adjusting leaves");
		src_node = find_leaf_at_level(tree.head, tree.depth, 0);
		right = src_node->parent->right;
		left = src_node->parent->left;
		src_node->parent->entry = right->entry;
		src_node->parent->left = src_node->parent->right = NULL;
		
		level = 2;
		while (!(dest_node = find_leaf_at_level(tree.head, tree.depth - level, 0)))
			level++;
		dest_node->left = left;
		dest_node->right = right;
		right->entry = dest_node->entry;
		dest_node->entry = NULL;
		left->parent = dest_node;
		right->parent = dest_node;
		tree.depth = 0;
		setup_huffman_tree(&tree, tree.head, 0, NULL);
	}
	
	/*
	 *  Find rightmost leaf and replace it with fake entry. This is needed
	 *  as the JPG standard doesn't allow huffman entries with all bits set
	 */
	node = tree.head->right;
	while (!node->entry)
		node = node->right;
	node->parent->right = NULL;
	TRACE("Exchanging rightmost leaf at level %d with fake leaf at level %d%s",
		node->entry->bits_length, fake_entry->bits_length, (node->entry == fake_entry ? "(fake leaf == rightmost leaf)" : ""));
	fake_entry->value = node->entry->value;
	
	/* Save generated huffman table */
	memset(list, 0, 17 * sizeof(HUFFMAN_NODE *));
	setup_huffman_tree(&tree, tree.head, 0, list);
	for (i = 1; i <= 16; i++) {
		num_codes[i] = 0;
		for (node = list[i]; node; node = node->next) {
			*value++ = node->entry->value;
			num_codes[i]++;
		}
	}
	
	destroy_huffman_tree(tree.head);
	
	return 0;
}


/* write_huffman_table:
 *  Writes an huffman table to the output stream and computes a lookup table
 *  for faster huffman encoding.
 */
static void
write_huffman_table(HUFFMAN_TABLE *table, unsigned const char *num_codes, unsigned const char *value)
{
	HUFFMAN_ENTRY *entry;
	int i, j, code, index;
	
	for (i = 1; i <= 16; i++)
		_jpeg_chunk_putc(num_codes[i]);
	memset(table, 0, sizeof(HUFFMAN_TABLE));
	index = code = 0;
	entry = table->entry;
	for (i = 1; i <= 16; i++) {
		for (j = 0; j < num_codes[i]; j++) {
			entry->value = value[index];
			entry->encoded_value = code;
			entry->bits_length = i;
			_jpeg_chunk_putc(value[index]);
			table->code[entry->value] = entry;
			entry++;
			code++;
			index++;
		}
		code <<= 1;
	}
}


/* write_header:
 *  Writes the complete header of a baseline JPG image to the output stream.
 *  This is made of the following chunks (in order): APP0, COM, DQT, SOF0,
 *  DHT, SOS.
 */
static int
write_header(int sampling, int greyscale, int quality, int width, int height)
{
	unsigned char *comment = (unsigned char *)"Generated using " JPGALLEG_VERSION_STRING;
	/* JFIF 1.1, no units, 1:1 aspect ratio, no thumbnail */
	unsigned char app0[] = { 'J', 'F', 'I', 'F', 0, 1, 1, 0, 0, 1, 0, 1, 0, 0 };
	/* 8 bits data precision, Y uses quantization table 0, Cb and Cr table 1 */
	unsigned char sof0[] = { 8, (height >> 8) & 0xff, height & 0xff, (width >> 8) & 0xff, width & 0xff, greyscale ? 1 : 3, 1, 0, 0, 2, 0x11, 1, 3, 0x11, 1 };
	/* Y uses DC table 0 and AC table 0, Cb and Cr use DC table 1 and AC table 1 */
	unsigned char sos_greyscale[] = { 1, 1, 0x00, 0, 63, 0 };
	unsigned char sos_color[] = { 3, 1, 0x00, 2, 0x11, 3, 0x11, 0, 63, 0 };
	
	_jpeg_putw(CHUNK_SOI);
	
	/* APP0 chunk */
	_jpeg_new_chunk(CHUNK_APP0);
	_jpeg_chunk_puts(app0, 14);
	_jpeg_write_chunk();
	
	/* COM chunk ;) */
	_jpeg_new_chunk(CHUNK_COM);
	_jpeg_chunk_puts(comment, (int) strlen((char*)comment));
	_jpeg_write_chunk();
	
	/* DQT chunk */
	_jpeg_new_chunk(CHUNK_DQT);
	_jpeg_chunk_putc(0);
	write_quantization_table(luminance_quant_table, default_luminance_quant_table, quality);
	if (!greyscale) {
		_jpeg_chunk_putc(1);
		write_quantization_table(chrominance_quant_table, default_chrominance_quant_table, quality);
	}
	_jpeg_write_chunk();
	
	/* SOF0 chunk */
	if (greyscale)
		sof0[7] = 0x11;
	else {
		switch (sampling) {
			case JPG_SAMPLING_411: sof0[7] = 0x22; break;
			case JPG_SAMPLING_422: sof0[7] = 0x21; break;
			case JPG_SAMPLING_444: sof0[7] = 0x11; break;
		}
	}
	_jpeg_new_chunk(CHUNK_SOF0);
	_jpeg_chunk_puts(sof0, greyscale ? 9 : 15);
	_jpeg_write_chunk();
	
	/* DHT chunk */
	_jpeg_new_chunk(CHUNK_DHT);
	_jpeg_chunk_putc(0x00);		/* DC huffman table 0 (used for luminance) */
	write_huffman_table(&_jpeg_huffman_dc_table[0], num_codes_dc_luminance, val_dc_luminance);
	_jpeg_chunk_putc(0x10);		/* AC huffman table 0 (used for luminance) */
	write_huffman_table(&_jpeg_huffman_ac_table[0], num_codes_ac_luminance, val_ac_luminance);
	if (!greyscale) {
		_jpeg_chunk_putc(0x01);		/* DC huffman table 1 (used for chrominance) */
		write_huffman_table(&_jpeg_huffman_dc_table[1], num_codes_dc_chrominance, val_dc_chrominance);
		_jpeg_chunk_putc(0x11);		/* AC huffman table 1 (used for chrominance) */
		write_huffman_table(&_jpeg_huffman_ac_table[1], num_codes_ac_chrominance, val_ac_chrominance);
	}
	_jpeg_write_chunk();
	
	/* SOS chunk */
	_jpeg_new_chunk(CHUNK_SOS);
	if (greyscale)
		_jpeg_chunk_puts(sos_greyscale, 6);
	else
		_jpeg_chunk_puts(sos_color, 10);
	_jpeg_write_chunk();
	
	return 0;
}


/* format_number:
 *  Computes the category and bits of a given number.
 */
static INLINE void
format_number(int num, int *category, int *bits)
{
	int abs_num, mask, cat;
	
	mask = num >> 31;
	abs_num = (num ^ mask) - mask;
	
	for (cat = 0; abs_num; cat++)
		abs_num >>= 1;
	*category = cat;
	if (num >= 0)
		*bits = num;
	else
		*bits = num + ((1 << cat) - 1);
}


/* put_bits:
 *   Writes some bits to the output stream.
 */
static INLINE int
put_bits(int value, int num_bits)
{
	int i;

	for (i = num_bits - 1; i >= 0; i--) {
		if (_jpeg_put_bit((value >> i) & 0x1))
			return -1;
	}
	return 0;
}


/* huffman_encode:
 *  Writes the huffman code of a given value.
 */
static INLINE int
huffman_encode(HUFFMAN_TABLE *table, int value)
{
	HUFFMAN_ENTRY *entry;
	
	if (current_pass == PASS_COMPUTE_HUFFMAN) {
		table->entry[value].value = value;
		table->entry[value].frequency++;
		return 0;
	}
	entry = table->code[value];
	if (entry)
		return put_bits(entry->encoded_value, entry->bits_length);
	TRACE("Huffman code (%d) not found", value);
	jpgalleg_error = JPG_ERROR_HUFFMAN;
	return -1;
}


/* encode_block:
 *  Encodes an 8x8 basic block of coefficients of given type (luminance or
 *  chrominance) and writes it to the output stream.
 */
static int
encode_block(short *block, int type, int *old_dc)
{
	HUFFMAN_TABLE *dc_table, *ac_table;
	int *quant_table;
	short data[64];
	int i, index;
	int value, num_zeroes;
	int category, bits;
	
	if (type == LUMINANCE) {
		dc_table = &_jpeg_huffman_dc_table[0];
		ac_table = &_jpeg_huffman_ac_table[0];
		quant_table = luminance_quant_table;
	}
	else {
		dc_table = &_jpeg_huffman_dc_table[1];
		ac_table = &_jpeg_huffman_ac_table[1];
		quant_table = chrominance_quant_table;
	}
	
	apply_fdct(block);
	
	zigzag_reorder(block, data);

	for (i = 0; i < 64; i++) {
		value = data[i];
		if (value < 0) {
			value = -value;
			value = (value * quant_table[i]) + (quant_table[i] >> 1);
			data[i] = -(value >> 19);
		}
		else
			data[i] = ((value * quant_table[i]) + (quant_table[i] >> 1)) >> 19;
	}
	
	value = data[0] - *old_dc;
	*old_dc = data[0];
	format_number(value, &category, &bits);
	if (huffman_encode(dc_table, category))
		return -1;
	if (put_bits(bits, category))
		return -1;

	num_zeroes = 0;
	for (index = 1; index < 64; index++) {
		if ((value = data[index]) == 0)
			num_zeroes++;
		else {
			while (num_zeroes > 15) {
				if (huffman_encode(ac_table, 0xf0))
					return -1;
				num_zeroes -= 16;
			}
			format_number(value, &category, &bits);
			value = (num_zeroes << 4) | category;
			if (huffman_encode(ac_table, value))
				return -1;
			if (put_bits(bits, category))
				return -1;
			num_zeroes = 0;
		}
	}
	if (num_zeroes > 0) {
		if (huffman_encode(ac_table, 0x00))
			return -1;
	}
	
	return 0;
}


/* _jpeg_c_rgb2ycbcr:
 *  C version of the RGB -> YCbCr color conversion routine. Converts 2 pixels
 *  at a time.
 */
static void
_jpeg_c_rgb2ycbcr(intptr_t addr, short *y1, short *cb1, short *cr1, short *y2, short *cb2, short *cr2)
{
	int r, g, b;
	unsigned int *ptr = (unsigned int *)addr;
	
	r = getr32(ptr[0]);
	g = getg32(ptr[0]);
	b = getb32(ptr[0]);
	*y1 = (((r * 76) + (g * 151) + (b * 29)) >> 8) - 128;
	*cb1 = (((r * -43) + (g * -85) + (b * 128)) >> 8);
	*cr1 = (((r * 128) + (g * -107) + (b * -21)) >> 8);
	r = getr32(ptr[1]);
	g = getg32(ptr[1]);
	b = getb32(ptr[1]);
	*y2 = (((r * 76) + (g * 151) + (b * 29)) >> 8) - 128;
	*cb2 = (((r * -43) + (g * -85) + (b * 128)) >> 8);
	*cr2 = (((r * 128) + (g * -107) + (b * -21)) >> 8);
}


/* encode_pass:
 *  Main encoding function. Can be used one time for unoptimized encoding,
 *  or two times for optimized encoding (first pass to gather sample
 *  frequencies to generate optimized huffman tables, second pass to
 *  actually write encoded image).
 */
static int
encode_pass(BITMAP *bmp, int quality)
{
	short y_buf[256], cb_buf[256], cr_buf[256];
	short y4[256], cb[64], cr[64], y_blocks_per_mcu;
	short *y_ptr, *cb_ptr, *cr_ptr;
	int dc_y, dc_cb, dc_cr;
	int block_x, block_y, x, y, i;
	intptr_t addr;
	
	_jpeg_io.buffer = _jpeg_io.buffer_start;
	
	if (write_header(sampling, greyscale, quality, bmp->w, bmp->h))
		return -1;
	
	dc_y = dc_cb = dc_cr = 0;
	
	for (block_y = 0; block_y < bmp->h; block_y += mcu_h) {
		for (block_x = 0; block_x < bmp->w; block_x += mcu_w) {
			addr = (intptr_t)fixed_bmp->line[block_y] + (block_x * 4);
			y_ptr = y_buf;
			cb_ptr = cb_buf;
			cr_ptr = cr_buf;
			for (y = 0; y < mcu_h; y++) {
				for (x = 0; x < mcu_w; x += 2) {
					rgb2ycbcr(addr, y_ptr, cb_ptr, cr_ptr, y_ptr + 1, cb_ptr + 1, cr_ptr + 1);
					y_ptr += 2;
					cb_ptr += 2;
					cr_ptr += 2;
					addr += 8;
				}
				addr += pitch;
			}
			if (mcu_w > 8) {
				if (mcu_h > 8) {
					/* 411 subsampling */
					for (y = 0; y < 8; y++) for (x = 0; x < 8; x++) {
						cb[(y << 3) | x] = (cb_buf[(y << 5) | (x << 1)] + cb_buf[(y << 5) | (x << 1) | 1] +
								   cb_buf[(y << 5) | (x << 1) | 16] + cb_buf[(y << 5) | (x << 1) | 17]) / 4;
						cr[(y << 3) | x] = (cr_buf[(y << 5) | (x << 1)] + cr_buf[(y << 5) | (x << 1) | 1] +
								   cr_buf[(y << 5) | (x << 1) | 16] + cr_buf[(y << 5) | (x << 1) | 17]) / 4;
						y4[(y << 3) | x] = y_buf[(y << 4) | x];
						y4[(y << 3) | x | 64] = y_buf[(y << 4) | x | 8];
						y4[(y << 3) | x | 128] = y_buf[(y << 4) | x | 128];
						y4[(y << 3) | x | 192] = y_buf[(y << 4) | x | 136];
						y_blocks_per_mcu = 4;
					}
				}
				else {
					/* 422 subsampling */
					for (y = 0; y < 8; y++) for (x = 0; x < 8; x++) {
						cb[(y << 3) | x] = (cb_buf[(y << 4) | (x << 1)] + cb_buf[(y << 4) | (x << 1) | 1]) / 2;
						cr[(y << 3) | x] = (cr_buf[(y << 4) | (x << 1)] + cr_buf[(y << 4) | (x << 1) | 1]) / 2;
						y4[(y << 3) | x] = y_buf[(y << 4) | x];
						y4[(y << 3) | x | 64] = y_buf[(y << 4) | x | 8];
						y_blocks_per_mcu = 2;
					}
				}
				y_ptr = y4;
				cb_ptr = cb;
				cr_ptr = cr;
			}
			else {
				/* 444 subsampling */
				y_ptr = y_buf;
				cb_ptr = cb_buf;
				cr_ptr = cr_buf;
				y_blocks_per_mcu = 1;
			}
			for (i = 0; i < y_blocks_per_mcu; i++) {
				if (encode_block(y_ptr, LUMINANCE, &dc_y))
					return -1;
				y_ptr += 64;
			}
			if (!greyscale) {
				if (encode_block(cb_ptr, CHROMINANCE, &dc_cb) ||
				    encode_block(cr_ptr, CHROMINANCE, &dc_cr))
					return -1;
			}
			if (progress_cb)
				progress_cb((progress_counter * 100) / progress_total);
			progress_counter++;
		}
	}
	
	_jpeg_flush_bits();
	_jpeg_putw(CHUNK_EOI);
	
	return 0;
}


/* do_encode:
 *  Encodes specified image in JPG format.
 */
static int
do_encode(BITMAP *bmp, AL_CONST RGB *pal, int quality, int flags, void (*callback)(int))
{
	unsigned char *tables = NULL;
	int i, result = -1;
	
	jpgalleg_error = JPG_ERROR_NONE;
	
	TRACE("############### Encode start ###############");
	
#ifdef JPGALLEG_MMX
	if (cpu_capabilities & CPU_MMX) {
		if (_rgb_r_shift_32 == 0)
			rgb2ycbcr = _jpeg_mmx_rgb2ycbcr;
		else if (_rgb_r_shift_32 == 16)
			rgb2ycbcr = _jpeg_mmx_bgr2ycbcr;
		else
			rgb2ycbcr = _jpeg_c_rgb2ycbcr;
	}
	else
#endif
	rgb2ycbcr = _jpeg_c_rgb2ycbcr;
	
	quality = MID(1, quality, 100);
	sampling = flags & 0xf;
	if ((sampling != JPG_SAMPLING_411) && (sampling != JPG_SAMPLING_422) && (sampling != JPG_SAMPLING_444)) {
		TRACE("Unknown sampling specified in flags parameter");
		return -1;
	}
	greyscale = flags & JPG_GREYSCALE;
	if (greyscale)
		sampling = JPG_SAMPLING_444;
	
	mcu_w = 8 * (sampling == JPG_SAMPLING_444 ? 1 : 2);
	mcu_h = 8 * (sampling != JPG_SAMPLING_411 ? 1 : 2);
	
	fixed_bmp = create_bitmap_ex(32, (bmp->w + mcu_w - 1) & ~(mcu_w - 1), (bmp->h + mcu_h - 1) & ~(mcu_h - 1));
	if (!fixed_bmp) {
		TRACE("Out of memory");
		return -1;
	}
	if (pal)
		select_palette(pal);
	blit(bmp, fixed_bmp, 0, 0, 0, 0, bmp->w, bmp->h);
	if (pal)
		unselect_palette();
	for (i = bmp->w; i < fixed_bmp->w; i++)
		blit(fixed_bmp, fixed_bmp, bmp->w - 1, 0, i, 0, 1, bmp->h);
	for (i = bmp->h; i < fixed_bmp->h; i++)
		blit(fixed_bmp, fixed_bmp, 0, bmp->h - 1, 0, i, fixed_bmp->w, 1);
	pitch = (int)(fixed_bmp->line[1] - fixed_bmp->line[0]) - (mcu_w * 4);
	
	progress_cb = callback;
	progress_counter = 0;
	progress_total = (fixed_bmp->w / mcu_w) * (fixed_bmp->h / mcu_h) * (flags & JPG_OPTIMIZE ? 2 : 1);
	
	TRACE("Saving %dx%d %s image in %s mode, quality %d%s", bmp->w, bmp->h, greyscale ? "greyscale" : "color",
		sampling == JPG_SAMPLING_444 ? "444" : (sampling == JPG_SAMPLING_422 ? "422" : "411"), quality,
		current_pass == PASS_COMPUTE_HUFFMAN ? " (first pass)" : "");
	
	if (flags & JPG_OPTIMIZE) {
		tables = calloc(1, 1092);
		if (!tables) {
			jpgalleg_error = JPG_ERROR_OUT_OF_MEMORY;
			goto exit_error;
		}
		num_codes_dc_luminance = tables;
		num_codes_dc_chrominance = tables + 17;
		num_codes_ac_luminance = tables + 34;
		num_codes_ac_chrominance = tables + 51;
		val_dc_luminance = tables + 68;
		val_dc_chrominance = tables + 324;
		val_ac_luminance = tables + 580;
		val_ac_chrominance = tables + 836;
		current_pass = PASS_COMPUTE_HUFFMAN;
		result = encode_pass(bmp, quality);
		if (result) {
			TRACE("First pass failed");
			goto exit_error;
		}
		result |= build_huffman_table(&_jpeg_huffman_dc_table[0], num_codes_dc_luminance, val_dc_luminance);
		result |= build_huffman_table(&_jpeg_huffman_ac_table[0], num_codes_ac_luminance, val_ac_luminance);
		if (!(flags & JPG_GREYSCALE)) {
			result |= build_huffman_table(&_jpeg_huffman_dc_table[1], num_codes_dc_chrominance, val_dc_chrominance);
			result |= build_huffman_table(&_jpeg_huffman_ac_table[1], num_codes_ac_chrominance, val_ac_chrominance);
		}
		if (result) {
			TRACE("Failed building optimized huffman tables");
			goto exit_error;
		}
	}
	else {
		num_codes_dc_luminance = (unsigned char *)default_num_codes_dc_luminance;
		num_codes_dc_chrominance = (unsigned char *)default_num_codes_dc_chrominance;
		num_codes_ac_luminance = (unsigned char *)default_num_codes_ac_luminance;
		num_codes_ac_chrominance = (unsigned char *)default_num_codes_ac_chrominance;
		val_dc_luminance = (unsigned char *)default_val_dc_luminance;
		val_dc_chrominance = (unsigned char *)default_val_dc_chrominance;
		val_ac_luminance = (unsigned char *)default_val_ac_luminance;
		val_ac_chrominance = (unsigned char *)default_val_ac_chrominance;
	}
	current_pass = PASS_WRITE;
	result = encode_pass(bmp, quality);

exit_error:
	destroy_bitmap(fixed_bmp);
	if (tables)
		free(tables);
	
	TRACE("################ Encode end ################");
	
	return result;
}


/* save_jpg:
 *  Saves specified BITMAP into a JPG file with quality 75, no subsampling
 *  and no progress callback.
 */
int
save_jpg(AL_CONST char *filename, BITMAP *bmp, AL_CONST RGB *palette)
{
	return save_jpg_ex(filename, bmp, palette, DEFAULT_QUALITY, DEFAULT_FLAGS, NULL);
}


/* save_jpg_ex:
 *  Saves a BITMAP into a JPG file using given quality, subsampling mode and
 *  progress callback.
 */
int
save_jpg_ex(AL_CONST char *filename, BITMAP *bmp, AL_CONST RGB *palette, int quality, int flags, void (*callback)(int progress))
{
	PACKFILE *f;
	PALETTE pal;
	int result, size;
	
	if (!palette)
		palette = pal;
	
	size = (bmp->w * bmp->h * 3) + 1000;    /* This extimation should be more than enough in all cases */
	_jpeg_io.buffer = _jpeg_io.buffer_start = (unsigned char *)malloc(size);
	_jpeg_io.buffer_end = _jpeg_io.buffer_start + size;
	if (!_jpeg_io.buffer) {
		TRACE("Out of memory");
		jpgalleg_error = JPG_ERROR_OUT_OF_MEMORY;
		return -1;
	}
	f = pack_fopen(filename, F_WRITE);
	if (!f) {
		TRACE("Cannot open %s for writing", filename);
		jpgalleg_error = JPG_ERROR_WRITING_FILE;
		free(_jpeg_io.buffer);
		return -1;
	}
	
	TRACE("Saving JPG to file %s", filename);
	
	result = do_encode(bmp, palette, quality, flags, callback);
	if (!result)
		pack_fwrite(_jpeg_io.buffer_start, _jpeg_io.buffer - _jpeg_io.buffer_start, f);
	
	free(_jpeg_io.buffer_start);
	pack_fclose(f);
	return result;
}


/* save_memory_jpg:
 *  Saves a BITMAP in JPG format and stores it into a memory buffer. The JPG
 *  is saved with quality 75, no subsampling and no progress callback.
 */
int
save_memory_jpg(void *buffer, int *size, BITMAP *bmp, AL_CONST RGB *palette)
{
	return save_memory_jpg_ex(buffer, size, bmp, palette, DEFAULT_QUALITY, DEFAULT_FLAGS, NULL);
}


/* save_memory_jpg_ex:
 *  Saves a BITMAP in JPG format using given quality and subsampling settings
 *  and stores it into a memory buffer.
 */
int
save_memory_jpg_ex(void *buffer, int *size, BITMAP *bmp, AL_CONST RGB *palette, int quality, int flags, void (*callback)(int progress))
{
	int result;
	
	if (!buffer) {
		TRACE("Invalid buffer pointer");
		return -1;
	}
	
	TRACE("Saving JPG to memory buffer at %p (size = %d)", buffer, *size);
	
	_jpeg_io.buffer = _jpeg_io.buffer_start = buffer;
	_jpeg_io.buffer_end = _jpeg_io.buffer_start + *size;
	*size = 0;
	
	result = do_encode(bmp, palette, quality, flags, callback);
	
	if (result == 0)
		*size = _jpeg_io.buffer - _jpeg_io.buffer_start;
	return result;
}
