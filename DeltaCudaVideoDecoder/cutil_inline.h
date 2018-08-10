#pragma once

// Beginning of GPU Architecture definitions
inline int _ConvertSMVer2CoresDrvApi(int major, int minor)
{
	// Defines for GPU Architecture types (using the SM version to determine the # of cores per SM
	typedef struct 
	{
		int SM; // 0xMm (hexidecimal notation), M = SM Major version, and m = SM minor version
		int Cores;
	} sSMtoCores;

    sSMtoCores nGpuArchCoresPerSM[] =
    {	
		{ 0x10,  8 },
        { 0x11,  8 },
        { 0x12,  8 },
        { 0x13,  8 },
        { 0x20, 32 },
        { 0x21, 48 },
        {   -1, -1 }
    };

	int index = 0;
	while (nGpuArchCoresPerSM[index].SM != -1) 
	{
		if (nGpuArchCoresPerSM[index].SM == ((major << 4) + minor) ) 
		{
			return nGpuArchCoresPerSM[index].Cores;
		}
		index++;
	}
	printf("MapSMtoCores undefined SMversion %d.%d!\n", major, minor);

	return -1;
}
// end of GPU Architecture definitions

// This function returns the best Graphics GPU based on performance
inline int cutilDrvGetMaxGflopsGraphicsDeviceId()
{
	CUdevice current_device = 0, max_perf_device = 0;
	int device_count     = 0, sm_per_multiproc = 0;
	int max_compute_perf = 0, best_SM_arch     = 0;
	int major = 0, minor = 0, multiProcessorCount, clockRate;
	int bTCC = 0, version;
	char deviceName[256];

	cuDeviceGetCount(&device_count);
	if (device_count <= 0)
	return -1;

	cuDriverGetVersion(&version);

	// Find the best major SM Architecture GPU device that are graphics devices
	while ( current_device < device_count ) 
	{
		cuDeviceGetName(deviceName, 256, current_device);
		cuDeviceComputeCapability(&major, &minor, current_device);

		if (version >= 3020) 
		{
			cuDeviceGetAttribute(&bTCC,  CU_DEVICE_ATTRIBUTE_TCC_DRIVER, current_device);
		} 
		else 
		{
			// Assume a Tesla GPU is running in TCC if we are running CUDA 3.1
			if (deviceName[0] == 'T') 
				bTCC = 1;
		}
		if (!bTCC) 
		{
			if (major > 0 && major < 9999) 
			{
				best_SM_arch = max(best_SM_arch, major);
			}
		}
		current_device++;
	}

	// Find the best CUDA capable GPU device
	current_device = 0;
	while( current_device < device_count ) 
	{
		cuDeviceGetAttribute(&multiProcessorCount, CU_DEVICE_ATTRIBUTE_MULTIPROCESSOR_COUNT, current_device);
		cuDeviceGetAttribute(&clockRate, CU_DEVICE_ATTRIBUTE_CLOCK_RATE, current_device);
		cuDeviceComputeCapability(&major, &minor, current_device);

		if (version >= 3020)
		{
			cuDeviceGetAttribute(&bTCC,  CU_DEVICE_ATTRIBUTE_TCC_DRIVER, current_device);
		} 
		else 
		{
			// Assume a Tesla GPU is running in TCC if we are running CUDA 3.1
			if (deviceName[0] == 'T') bTCC = 1;
		}

		if (major == 9999 && minor == 9999) 
		{
			sm_per_multiproc = 1;
		} 
		else 
		{
			sm_per_multiproc = _ConvertSMVer2CoresDrvApi(major, minor);
		}

		// If this is a Tesla based GPU and SM 2.0, and TCC is disabled, this is a contendor
		if (!bTCC) // Is this GPU running the TCC driver?  If so we pass on this
		{
			int compute_perf = multiProcessorCount * sm_per_multiproc * clockRate;
			if(compute_perf > max_compute_perf) 
			{
				// If we find GPU with SM major > 2, search only these
				if (best_SM_arch > 2) 
				{
					// If our device = dest_SM_arch, then we pick this one
					if (major == best_SM_arch) 
					{	
						max_compute_perf  = compute_perf;
						max_perf_device   = current_device;
					}
				} 
				else 
				{
					max_compute_perf  = compute_perf;
					max_perf_device   = current_device;
				}
			}

#ifdef DEBUG
			cuDeviceGetName(deviceName, 256, current_device);
			DbgLog((LOG_TRACE, 10, L"CUDA Device: %S, Compute: %d.%d, CUDA Cores: %d, Clock: %d MHz", deviceName, major, minor, multiProcessorCount * sm_per_multiproc, clockRate / 1000));
#endif
		}
		++current_device;
	}

	return max_perf_device;
}