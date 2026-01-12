/// Custom distance field block compression
/// Code from https://graphicsprogrammingconference.com/archive/2025/#how-to-decimate-your-textures-bcn-compression-tricks-in-horizon-forbidden-west

/*
	16 bit scalar to BC1 encoder.  The main entrypoint is:
		void sBuildBC1Block(uint64_t *outBCBlock, std::vector<float> inInputData, ::glm::vec3 inUnpackDot)

	To convert this to your codebase, the following will need to be replaced with your version:
		::glm::vec3 - 3D vector, like float3
		::int3 - 3D integer, like int3 or uint3
		assert
		std::vector<type> - Like std::span
		std::array<type, integer size> - Fixed size array.  A C-style array on the stack, or std::array
*/


/**
@brief Extend a 16-bit/RGB565 color to 24-bit/RGB888.  Replicates GPU behaviour.
**/
int3 sConvert565To888(uint in565Color)
{
    uint channel_r = (in565Color >> 11) & 31;
    uint channel_g = (in565Color >> 5) & 63;
    uint channel_b = (in565Color) & 31;

    return int3(
		(channel_r << 3) | (channel_r >> 2),
		(channel_g << 2) | (channel_g >> 4),
		(channel_b << 3) | (channel_b >> 2));
}


/**
@brief Unpack an RGB888 to a scalar.
	Factored out into shared code, because doing the dotproduct and /255 in different orders causes slight differences...
**/
float sUnpackRGB888ToScalar(int3 inRGB888, float3 inUnpackDot)
{
    float3 color_unit = float3(inRGB888) / 255.0f;
    float unpacked_value = dot(color_unit, inUnpackDot);
    return unpacked_value;
}


/**
@brief Unpack a 16-bit/RGB565 color to a float.
**/
static float sEvaluateEndpointColor(uint inColor, float3 inUnpackDot)
{
    return sUnpackRGB888ToScalar(sConvert565To888(inColor), inUnpackDot);
}


/**
@brief Find the four interpolated color for a BC1 block, replicating the values sampled by AMD GPUs.
	Input endpoint colors are expected to be 16-bit RGB565.
	Output colors are RGB888.

	Based on https://fgiesen.wordpress.com/2021/10/04/gpu-bcn-decoding/
**/
static void sBuildInterpolatedColors_AMD(uint inEndpointColor0,
                                         uint inEndpointColor1,
out int3 colors[4])
{

    int3 color_0 = sConvert565To888(inEndpointColor0);
    int3 color_1 = sConvert565To888(inEndpointColor1);

    if (inEndpointColor0 > inEndpointColor1)
    {
        colors[0] = color_0;
        colors[1] = color_1;
        colors[2] = ((color_0 * 43) + (color_1 * 21) + ::
        int3(32, 32, 32)) / 64;
        colors[3] = ((color_0 * 21) + (color_1 * 43) + ::
        int3(32, 32, 32)) / 64;
    }
    else
    {
        colors[0] = color_0;
        colors[1] = color_1;
        colors[2] = (color_0 + color_1 + ::
        int3(1, 1, 1)) / 2;
        colors[3] = ::
        int3(0, 0, 0); // We're ignoring alpha
    }
}


/**
@brief Find the four interpolated color for a BC1 block, replicating the values sampled by nVidia GPUs.
	Input endpoint colors are expected to be 16-bit RGB565.
	Output colors are RGB888.

	Based on https://fgiesen.wordpress.com/2021/10/04/gpu-bcn-decoding/
**/
static void sBuildInterpolatedColors_nVidia(uint inEndpointColor0,
                                            uint inEndpointColor1,
											out int3 colors[4]
											)
{
    int3 color_0 = sConvert565To888(inEndpointColor0);
    int3 color_1 = sConvert565To888(inEndpointColor1);

	// Extract 5-bit red and blue colors from input RGB565
    uint channel_0_r = (inEndpointColor0 >> 11) & 31;
    uint channel_0_b = (inEndpointColor0) & 31;

    uint channel_1_r = (inEndpointColor1 >> 11) & 31;
    uint channel_1_b = (inEndpointColor1) & 31;


    if (inEndpointColor0 > inEndpointColor1)
    {
        colors[0] = color_0;
        colors[1] = color_1;

		// Red and  blue channels: input is 5-bit, output is 8 bit
        colors[2].x = ((2 * channel_0_r + channel_1_r) * 22) >> 3;
        colors[2].z = ((2 * channel_0_b + channel_1_b) * 22) >> 3;

        colors[3].x = ((channel_0_r + 2 * channel_1_r) * 22) >> 3;
        colors[3].z = ((channel_0_b + 2 * channel_1_b) * 22) >> 3;


		// Green channel: input is 8 bit
        int diff = color_1.y - color_0.y;
        int scaled_diff = 80 * diff + (diff >> 2);
        colors[2].y = color_0.y + ((128 + scaled_diff) >> 8);
        colors[3].y = color_1.y + ((128 - scaled_diff) >> 8);
    }
    else
    {
        colors[0] = color_0;
        colors[1] = color_1;

		// Red and  blue channels: input is 5-bit, output is 8 bit
        colors[2].x = ((channel_0_r + channel_1_r) * 33) >> 3;
        colors[2].z = ((channel_0_b + channel_1_b) * 33) >> 3;

		// Green channel: input is 8 bit
        int diff = color_1.y - color_0.y;
        int scaled_diff = 128 * diff + (diff >> 2);
        colors[2].y = color_0.y + ((128 * scaled_diff) >> 8);

        colors[3] = int3(0, 0, 0); // We're ignoring alpha
    }
}


/**
@brief Find the four interpolated color for a BC1 block, replicating the values sampled by Intel GPUs.
	Input endpoint colors are expected to be 16-bit RGB565.
	Output colors are RGB888.

	Based on https://fgiesen.wordpress.com/2021/10/04/gpu-bcn-decoding/
**/
static void sBuildInterpolatedColors_Intel(uint inEndpointColor0,
                                           uint inEndpointColor1, out int3 colors[4])
{
    int3 color_0 = sConvert565To888(inEndpointColor0);
    int3 color_1 = sConvert565To888(inEndpointColor1);

    if (inEndpointColor0 > inEndpointColor1)
    {
        colors[0] = color_0;
        colors[1] = color_1;
        colors[2] = ((color_0 * 171) + (color_1 * 85) + int3(128, 128, 128)) / 256;
        colors[3] = ((color_0 * 85) + (color_1 * 171) + int3(128, 128, 128)) / 256;
    }
    else
    {
        colors[0] = color_0;
        colors[1] = color_1;
        colors[2] = ((color_0 * 128) + (color_1 * 128) + int3(128, 128, 128)) / 256;
        colors[3] = int3(0, 0, 0); // We're ignoring alpha
    }
}


/**
@brief Given a 4x4 list of input values, and a pair of endpoint colors, build a final 64-bit BC1 block
	Input values are expected to be in [0, 1].
	Outputs the error and the BC block
**/
static void sBuildBC1BlockAndEvaluateError(float inInputData[16], uint inEndpointColorLo,
                                           uint inEndpointColorHi, float3 inUnpackDot, out float netError, out uint2 bcBlock)
{

	// Set endpoints.  
    uint mEndpoint0 = max(inEndpointColorLo, inEndpointColorHi);
    uint mEndpoint1 = min(inEndpointColorLo, inEndpointColorHi);

	// Find the four interpolated colors

    int3 interp_amd[4], interp_nv[4], interp_intel[4];

    sBuildInterpolatedColors_AMD(mEndpoint0, mEndpoint1, interp_amd);
    sBuildInterpolatedColors_nVidia(mEndpoint0, mEndpoint1, interp_nv);
    sBuildInterpolatedColors_Intel(mEndpoint0, mEndpoint1, interp_intel);

    float interpValues[4];
    for (int i = 0; i < 4; i++)
    {
        interpValues[i] = max(
							sUnpackRGB888ToScalar(interp_amd[i], inUnpackDot),
						  max(
							sUnpackRGB888ToScalar(interp_nv[i], inUnpackDot),
							sUnpackRGB888ToScalar(interp_intel[i], inUnpackDot)));
    }

    uint mIndices = 0;
    float net_error = 0.0f;

	// Choose the 2-bit index for each of the 4x4 values
    for (int source_index = 0; source_index < 16; source_index++)
    {
        float source_value = inInputData[source_index];

        float least_error = 1e10f;
        uint best_interpolated_index = 0;

        for (int i = 0; i < 4; i++)
        {
            float delta = source_value - interpValues[i];
            if ((delta >= 0.0f) && (delta < least_error))
            {
				// Interpolated value is lower than source, and improves error
                least_error = delta;
                best_interpolated_index = i;
            }
        }

        net_error += least_error * least_error; // Accumulate squared error

        mIndices |= best_interpolated_index << (source_index * 2);
    }
    netError = net_error;

    //Store as uint2 so we have 64 bits
    bcBlock.x = (mEndpoint1 << 16) | mEndpoint0;
    bcBlock.y = mIndices;
}


/**
@brief Find the highest 16-bit endpoint color that's <= the input value.
	This will quickly find a close approximation, but it's possible that it's not the best.
	A lookup table may be better ...
**/
static uint sFindEndpointColor(float inValue, float3 inUnpackDot)
{
	// The code below sets up the components in order from most to least significant.
    uint endpoint_color = 0;

    for (int r_step = 16; r_step > 0; r_step /= 2) // R is in [0, 31]
    {
        uint hypothetical_endpoint_color = endpoint_color + (r_step << 11);
        if (sEvaluateEndpointColor(hypothetical_endpoint_color, inUnpackDot) <= inValue)
            endpoint_color = hypothetical_endpoint_color;
    }
    for (int g_step = 32; g_step > 0; g_step /= 2) // G is in [0, 63]
    {
        uint hypothetical_endpoint_color = endpoint_color + (g_step << 5);
        if (sEvaluateEndpointColor(hypothetical_endpoint_color, inUnpackDot) <= inValue)
            endpoint_color = hypothetical_endpoint_color;
    }
    for (int b_step = 16; b_step > 0; b_step /= 2) // B is in [0, 31]
    {
        uint hypothetical_endpoint_color = endpoint_color + b_step;
        if (sEvaluateEndpointColor(hypothetical_endpoint_color, inUnpackDot) <= inValue)
            endpoint_color = hypothetical_endpoint_color;
    }
    return endpoint_color;
}


/**
@brief BC1 block compression entrypoint.  Input is a 4x4 set of values in [0, 1]; output is a 64-bit BC1 block.
		A good value for inUnpackDot is (0.96414679f, 0.03518212f, 0.00067109f).

	Searches different endpoint choices to find the encoding with least error for inInputData.
	This implementation is best suited to input values that cover a relatively small range of the domain - such as when compressing smooth heightfields and distance fields.
	It may not give good results with other data.
**/
void sBuildBC1Block(float inInputData[16], float3 inUnpackDot, out uint2 bcBlock)
{
	// Find minimum and maximum input value
	float2 input_value_range = float2(+1e10f, -1e10f);
    [unroll]
	for (int i = 0; i < 16; i++)
    {
        input_value_range.x = min(input_value_range.x, inInputData[i]);
        input_value_range.y = max(input_value_range.y, inInputData[i]);
    }

	// Find 16-bit endpoints.  The two should be in order, and not equal.
    uint endpoint_for_range_min = min(sFindEndpointColor(input_value_range.x, inUnpackDot), (65535 - 1));
	// endpoint_for_range_max>min, so min can't be 65535
    uint endpoint_for_range_max = max(sFindEndpointColor(input_value_range.y, inUnpackDot), (endpoint_for_range_min + 1));

    float least_error = 1e10f;
    uint2 best_bc_block = uint2(0,0);


    for (uint hypothetical_endpoint_hi = endpoint_for_range_min + 1; hypothetical_endpoint_hi <= endpoint_for_range_max;
	     hypothetical_endpoint_hi++)
    {
        float curr_error = 1e10f;
        uint2 curr_bc_block;
        sBuildBC1BlockAndEvaluateError(inInputData, endpoint_for_range_min,
		                               hypothetical_endpoint_hi, inUnpackDot, curr_error, curr_bc_block);
        if (curr_error < least_error)
        {
            least_error = curr_error;
            best_bc_block = curr_bc_block;
        }
    }

	bcBlock = best_bc_block;
}
