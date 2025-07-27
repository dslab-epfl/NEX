# VTA extracted performance data

# Filename to label mapping
filename_to_label_map = {
    'resnet34_rtl_v1.txt': 'VTA-NEX-resnet34_v1-rtl',
    'resnet18_rtl_v1.txt': 'VTA-NEX-resnet18_v1-rtl',
    'classify_multi-4-resnet18_rtl_v1.txt': 'VTA-NEX-4devices-resnet18_v1-rtl',
    'yolov3_rtl.txt': 'VTA-NEX-yolov3-rtl',
    'resnet18_legacy_dsim_v1.txt': 'VTA-NEX-resnet18_v1-dsim',
    'yolov3_legacy_dsim.txt': 'VTA-NEX-yolov3-dsim',
    'classify_multi-8-resnet18_dsim_v1.txt': 'VTA-NEX-8devices-resnet18_v1-dsim',
    'mm_legacy_dsim.txt': 'VTA-NEX-matmul-dsim',
    'classify_multi-4-resnet18_dsim_v1.txt': 'VTA-NEX-4devices-resnet18_v1-dsim',
    'mm_rtl.txt': 'VTA-NEX-matmul-rtl',
    'classify_multi-8-resnet18_rtl_v1.txt': 'VTA-NEX-8devices-resnet18_v1-rtl',
    'resnet50_rtl_v1.txt': 'VTA-NEX-resnet50_v1-rtl',
    'resnet50_legacy_dsim_v1.txt': 'VTA-NEX-resnet50_v1-dsim',
    'resnet34_legacy_dsim_v1.txt': 'VTA-NEX-resnet34_v1-dsim',
}

# Combined data as dictionary
performance_data = {
    'VTA-NEX-resnet34_v1-rtl': {
        'latency': 59292000.0,
        'real_time': 41.02017693396192,
        'filename': 'resnet34_rtl_v1.txt'
    },
    'VTA-NEX-resnet18_v1-rtl': {
        'latency': 54273000.0,
        'real_time': 21.18925173697062,
        'filename': 'resnet18_rtl_v1.txt'
    },
    'VTA-NEX-4devices-resnet18_v1-rtl': {
        'latency': 245560000.0,
        'real_time': 279.56750557094347,
        'filename': 'classify_multi-4-resnet18_rtl_v1.txt'
    },
    'VTA-NEX-yolov3-rtl': {
        'latency': 159315000.0,
        'real_time': 97.86052637698594,
        'filename': 'yolov3_rtl.txt'
    },
    'VTA-NEX-resnet18_v1-dsim': {
        'latency': 53573000.0,
        'real_time': 2.9126296780304983,
        'filename': 'resnet18_legacy_dsim_v1.txt'
    },
    'VTA-NEX-yolov3-dsim': {
        'latency': 163425000.0,
        'real_time': 6.885595316998661,
        'filename': 'yolov3_legacy_dsim.txt'
    },
    'VTA-NEX-8devices-resnet18_v1-dsim': {
        'latency': 157003000.0,
        'real_time': 44.84243744192645,
        'filename': 'classify_multi-8-resnet18_dsim_v1.txt'
    },
    'VTA-NEX-matmul-dsim': {
        'latency': 234481000.0,
        'real_time': 47.26330394891556,
        'filename': 'mm_legacy_dsim.txt'
    },
    'VTA-NEX-4devices-resnet18_v1-dsim': {
        'latency': 240532000.0,
        'real_time': 51.387042318005115,
        'filename': 'classify_multi-4-resnet18_dsim_v1.txt'
    },
    'VTA-NEX-matmul-rtl': {
        'latency': 206446000.0,
        'real_time': 862.0423392710509,
        'filename': 'mm_rtl.txt'
    },
    'VTA-NEX-8devices-resnet18_v1-rtl': {
        'latency': 133989000.0,
        'real_time': 212.39885768003296,
        'filename': 'classify_multi-8-resnet18_rtl_v1.txt'
    },
    'VTA-NEX-resnet50_v1-rtl': {
        'latency': 683264000.0,
        'real_time': 2331.7995840079384,
        'filename': 'resnet50_rtl_v1.txt'
    },
    'VTA-NEX-resnet50_v1-dsim': {
        'latency': 707308000.0,
        'real_time': 34.77159382402897,
        'filename': 'resnet50_legacy_dsim_v1.txt'
    },
    'VTA-NEX-resnet34_v1-dsim': {
        'latency': 65877000.0,
        'real_time': 5.252614709082991,
        'filename': 'resnet34_legacy_dsim_v1.txt'
    },
}
