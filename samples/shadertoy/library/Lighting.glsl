/*
	Lighting models
*/

#include "Math.glsl"


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
							  const float3 surfNorm, const float3 vH, const float r0, const float roughness)
{
	const float	E	= 2.7182818284;
	float		nh	= Dot( surfNorm, vH );
	float		nv	= Dot( surfNorm, viewDir );
	float		nl	= Dot( surfNorm, lightDir );
	float		r2	= roughness * roughness;
	float		nh2	= nh * nh;
	float		ex	= - (1.0 - nh2) / (nh2 * r2);
	float		d	= Pow( E, ex ) / (r2 * nh2 * nh2);
	float		f	= Lerp( Pow( 1.0 - nv, 5.0 ), 1.0, r0 );
	float		x	= 2.0 * nh / Dot( viewDir, vH );
	float		g	= Min( 1.0, Min( x * nl, x * nv ) );
	float		ct	= d*f*g / nv;

	LightingResult	res;
	res.diffuse  = diffuse * Max( 0.0, nl );
	res.specular = specular * Max( 0.0, ct );
	return res;
}



//-----------------------------------------------------------------------------
// Attenuation

float _GGX (const float dotNH, const float roughness)
{
	float alpha = roughness * roughness;
	float alpha2 = alpha * alpha;
	float denom = dotNH * dotNH * (alpha2 - 1.0) + 1.0;
	return (alpha2) / (Pi() * denom*denom); 
}

float _SchlicksmithGGX (const float dotNL, const float dotNV, const float roughness)
{
	float r = (roughness + 1.0);
	float k = (r*r) / 8.0;
	float GL = dotNL / (dotNL * (1.0 - k) + k);
	float GV = dotNV / (dotNV * (1.0 - k) + k);
	return GL * GV;
}

float3 _Schlick (const float3 albedo, const float cosTheta, const float metallic)
{
	float3 F0 = Lerp( float3(0.04), albedo, metallic ); // * material.specular
	float3 F = F0 + (1.0 - F0) * Pow(1.0 - cosTheta, 5.0); 
	return F;    
}

float3 BRDF (const float3 lightColor, const float3 albedo, const float3 lightDir, const float3 viewDir,
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
		float  r = Max( 0.05, roughness );
		float  D = _GGX( dotNH, r ); 
		float  G = _SchlicksmithGGX( dotNL, dotNV, r );
		float3 F = _Schlick( albedo, dotNV, metallic );

		float3 spec = D * F * G / (4.0 * dotNL * dotNV);

		color += spec * dotNL * lightColor;
	}

	return color;
}



//-----------------------------------------------------------------------------
// Attenuation

float LinearAttenuation (const float dist, const float radius)
{
	return Saturate( 1.0 - (dist / radius) );
}

float QuadraticAttenuation (const float dist, const float radius)
{
	float	f = dist / radius;
	return Saturate( 1.0 - f*f );
}

float Attenuation (const float3 attenFactor, const float dist)
{
	return 1.0 / ( attenFactor.x + attenFactor.y * dist + attenFactor.z * dist * dist );
}