# JPEG extracted performance data

# Filename to label mapping
filename_to_label_map = {
    'jpeg_seq_rtl.log': 'JPEG-NEX-single-rtl',
    'jpeg_multi_post_2_dsim.log': 'JPEG-NEX-2devices-multi-dsim',
    'jpeg_multi_post_4_dsim.log': 'JPEG-NEX-4devices-multi-dsim',
    'jpeg_multi_post_2_rtl.log': 'JPEG-NEX-2devices-multi-rtl',
    'jpeg_seq_dsim_legacy.log': 'JPEG-NEX-single-dsim',
    'jpeg_multi_post_4_rtl.log': 'JPEG-NEX-4devices-multi-rtl',
    'jpeg_multi_post_8_rtl.log': 'JPEG-NEX-8devices-multi-rtl',
    'jpeg_multi_post_8_dsim.log': 'JPEG-NEX-8devices-multi-dsim',
}

# Combined data as dictionary
performance_data = {
    'JPEG-NEX-single-rtl': {
        'latency': 1339089000.0,
        'real_time': 1756.954638,
        'filename': 'jpeg_seq_rtl.log'
    },
    'JPEG-NEX-2devices-multi-dsim': {
        'latency': 989270000.0,
        'real_time': 195.106645,
        'filename': 'jpeg_multi_post_2_dsim.log'
    },
    'JPEG-NEX-4devices-multi-dsim': {
        'latency': 656117000.0,
        'real_time': 137.670186,
        'filename': 'jpeg_multi_post_4_dsim.log'
    },
    'JPEG-NEX-2devices-multi-rtl': {
        'latency': 1004606000.0,
        'real_time': 1293.18854,
        'filename': 'jpeg_multi_post_2_rtl.log'
    },
    'JPEG-NEX-single-dsim': {
        'latency': 1351726000.0,
        'real_time': 162.892544,
        'filename': 'jpeg_seq_dsim_legacy.log'
    },
    'JPEG-NEX-4devices-multi-rtl': {
        'latency': 653604000.0,
        'real_time': 817.898002,
        'filename': 'jpeg_multi_post_4_rtl.log'
    },
    'JPEG-NEX-8devices-multi-rtl': {
        'latency': 472816000.0,
        'real_time': 584.133853,
        'filename': 'jpeg_multi_post_8_rtl.log'
    },
    'JPEG-NEX-8devices-multi-dsim': {
        'latency': 480842000.0,
        'real_time': 147.014552,
        'filename': 'jpeg_multi_post_8_dsim.log'
    },
}
