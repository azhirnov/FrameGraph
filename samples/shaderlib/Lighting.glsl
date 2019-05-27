/*
	Lighting models
*/

#include "Math.glsl"


//-----------------------------------------------------------------------------
// Attenuation

float  LinearAttenuation (const float dist, const float radius)
{
	return Saturate( 1.0 - (dist / radius) );
}

float  QuadraticAttenuation (const float dist, const float radius)
{
	float	f = dist / radius;
	return Saturate( 1.0 - f*f );
}

float  Attenuation (const float3 attenFactor, const float dist)
{
	return 1.0 / ( attenFactor.x + attenFactor.y * dist + attenFactor.z * dist * dist );
}


//-----------------------------------------------------------------------------
// Lighting
/*
struct LightingResult
{
	float3	diffuse;
	float3	specular;
};


LightingResult  Lambert (const float3 diffuse, const float3 lightDir, const float3 surfNorm)
{
	LightingResult	res;
	res.diffuse  = diffuse * Max( Dot( surfNorm, lightDir ), 0.0 );
	res.specular = float3(0.0);
	return res;
}


LightingResult  Phong (const float3 diffuse, const float3 specular, const float specPow,
					   const float3 lightDir, const float3 viewDir, const float3 surfNorm)
{
	LightingResult	res;
	float3	ref  = Reflect( viewDir, surfNorm );
	res.diffuse  = diffuse * Max( Dot( surfNorm, lightDir ), 0.0 );
	res.specular = specular * Pow( Max( Dot( lightDir, ref ), 0.0 ), specPow );
	return res;
}


LightingResult  Blinn (const float3 diffuse, const float3 specular, const float specPow, const float3 lightDir,
					   const float3 viewDir, const float3 surfNorm, const float3 halfwayVec)
{
	LightingResult	res;
	res.diffuse  = diffuse * Max( Dot( surfNorm, lightDir ), 0.0 );
	res.specular = specular * Pow( Max( Dot( surfNorm, halfwayVec ), 0.0 ), specPow );
	return res;
}


LightingResult  CookTorrance (const float3 diffuse, const float3 specular, const float3 lightDir, const float3 viewDir,
							  const float3 surfNorm, const float r0, const float roughness)
{
	const float	E	= 2.7182818284;
	float3		H 	= Normalize( lightDir + viewDir );
	float		nh	= Dot( surfNorm, H );
	float		nv	= Dot( surfNorm, viewDir );
	float		nl	= Dot( surfNorm, lightDir );
	float		r2	= roughness * roughness;
	float		nh2	= nh * nh;
	float		ex	= - (1.0 - nh2) / (nh2 * r2);
	float		d	= Pow( E, ex ) / (r2 * nh2 * nh2);
	float		f	= Lerp( Pow( 1.0 - nv, 5.0 ), 1.0, r0 );
	float		x	= 2.0 * nh / Dot( viewDir, H );
	float		g	= Min( 1.0, Min( x * nl, x * nv ) );
	float		ct	= d * f * g / nv;

	LightingResult	res;
	res.diffuse  = diffuse * Max( 0.0, nl );
	res.specular = specular * Max( 0.0, ct );
	return res;
}


float3  DiffuseLighting (const float3 diffuseColor, const float3 surfNorm, const float3 lightDir)
{
	float NdotL = Max( 0.0, Dot( surfNorm, lightDir ));
	return diffuseColor * NdotL;
}

float3  SpecularLighting (const float3 diffuseColor, const float4 spec, const float3 surfNorm, const float3 lightDir, const float3 viewDir)
{
	// Phong lighting.
	float3	R 			= Normalize( Reflect( -lightDir, surfNorm ));
	float 	RdotV 		= Max( 0.0, Dot( R, viewDir ));

	// Blinn-Phong lighting
	float3	H 			= Normalize( lightDir + viewDir );
	float 	NdotH 		= Max( 0.0, Dot( surfNorm, H ));

	return diffuseColor * spec.xyz * Pow( RdotV, spec.w );
}

LightingResult  PointLight (const float3 lightPos, const float3 worldPos, const float3 viewDir, const float3 surfNorm,
							const float3 diffuseColor, const float4 spec, const float3 attenuation, const float roughness)
{
	float3	lightDir= lightPos - worldPos;
	float	dist 	= Length( lightDir );
	float	atten 	= Max( 0.0, Attenuation( attenuation, dist ));	lightDir /= dist;
		
	LightingResult res;
	res.diffuse 	= DiffuseLighting( diffuseColor, surfNorm, lightDir ) * atten;
	res.specular 	= SpecularLighting( diffuseColor, spec, surfNorm, lightDir, viewDir ) * atten;
	return res;
}
*/

//-----------------------------------------------------------------------------
// PBR

float3 SpecularBRDF (const float3 albedo, const float3 lightColor, const float3 lightDir, const float3 viewDir,
					 const float3 surfNorm, const float metallic, const float roughness)
{
	float3 H = Normalize( viewDir + lightDir );
	float dotNV = Saturate( Dot( surfNorm, viewDir ));
	float dotNL = Saturate( Dot( surfNorm, lightDir ));
	float dotLH = Saturate( Dot( lightDir, H ));
	float dotNH = Saturate( Dot( surfNorm, H ));

	float3 color = float3(0.0);

	if ( dotNL > 0.0 )
	{
		float  rough = Max( 0.05, roughness );

		float  D;
		{
			float alpha = rough * rough;
			float alpha2 = alpha * alpha;
			float denom = dotNH * dotNH * (alpha2 - 1.0) + 1.0;
			D = (alpha2) / (Pi() * denom*denom); 
		}

		float  G;
		{
			float r = (rough + 1.0);
			float k = (r*r) / 8.0;
			float GL = dotNL / (dotNL * (1.0 - k) + k);
			float GV = dotNV / (dotNV * (1.0 - k) + k);
			G = GL * GV;
		}

		float3 F;
		{
			float3 F0 = Lerp( float3(0.04), albedo, metallic );
			F = F0 + (1.0 - F0) * Pow(1.0 - dotNV, 5.0); 
		}

		float3 spec = D * F * G / (4.0 * dotNL * dotNV);

		color += spec * dotNL * lightColor;
	}

	return color;
}
