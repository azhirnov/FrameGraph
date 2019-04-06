// from https://www.shadertoy.com/view/4tGXWW

#define MAXRAYS 30
#define MAXBOUNCES 10
#define PI 3.142
#define INF 100000.0
#define BRIGHTNESS 1.0

#define R(p,a) p=cos(a)*p+sin(a)*vec2(-p.y,p.x);
    
struct Ray {
	vec3 origin;
	vec3 dir;
};

// A camera. Has a position and a direction. 
struct Camera {
    vec3 pos;
    Ray ray;
};

struct Sphere {
	vec3 pos;
	float radius;
};
    
struct Box {
	vec3 pos;
	vec3 size;
};

struct HitTest {
	bool hit;
    //bool emissive;
	float dist;
    vec3 normal;
    vec4 col;
    float ref;
};
    
float t;
float divergence;
    
#define NOHIT HitTest(false, INF, vec3(0), vec4(0), 0.0)
    
HitTest minT(in HitTest a, in HitTest b) {
    if (a.dist < b.dist) { return a; } else { return b; }
}

HitTest minT(in HitTest a, in HitTest b, in HitTest c) {
    return minT(a, minT(b, c));
}

HitTest minT(in HitTest a, in HitTest b, in HitTest c, in HitTest d) {
    return minT(a, minT(b, c, d));
}

HitTest minT(in HitTest a, in HitTest b, in HitTest c, in HitTest d, in HitTest e) {
    return minT(minT(a,b), minT(c,d,e));
}

HitTest intersectFloor(in Ray r) {
    if (r.dir.y >= 0.0) { return NOHIT; }
    return HitTest(true, r.origin.y / -r.dir.y, vec3(0,1,0), vec4(0), 0.0);
}

HitTest intersectBox(in Ray r, in Box b) {
 	// box, 0 on y, +/-10 on x, +20 on y
   // vec3 p = vec3(0);
   // vec3 s = vec3(30);
    b.size *= 0.5;
    vec3 ba = b.pos-b.size, bb = b.pos+b.size;
    
    HitTest h = NOHIT;
    float d = INF;
    
    //r.origin -= p;
    
    vec3 dA = (r.origin - ba) / -r.dir;
    vec3 dB = (r.origin - bb) / -r.dir;
    
    dA.x = dA.x <= 0.0 ? INF : dA.x;
    dA.y = dA.y <= 0.0 ? INF : dA.y;
    dA.z = dA.z <= 0.0 ? INF : dA.z;
    dB.x = dB.x <= 0.0 ? INF : dB.x;
    dB.y = dB.y <= 0.0 ? INF : dB.y;
    dB.z = dB.z <= 0.0 ? INF : dB.z;
    
    float d1 = min(dA.x, min(dA.y, dA.z));
    float d2 = min(dB.x, min(dB.y, dB.z));
    
    d = min(d1, d2);
    
    vec3 endPoint = r.origin + r.dir * d;
    endPoint -= b.pos;
    //endPoint = abs(endPoint);
    
    
    if (d != INF) {
        h.hit = true;
        //h.emissive = false;
        h.dist = d;
        h.ref = 0.0;
        
        if (abs(abs(endPoint.x) - b.size.x) < 0.01) {
            bool l = endPoint.x < 0.0;
       		h.normal = vec3(l ? 1 : -1,0,0);
        	h.col = l ? vec4(0.9,0.5,0.5,0) : vec4(0.5,0.5,0.9,0);
    		return h;
        }
        if (abs(abs(endPoint.z) - b.size.z) < 0.01) {
       		h.normal = vec3(0,0,-sign(endPoint.z));
        	h.col = vec4(0.9, 0.9, 0.9, 0.0);
           // h.ref = 0.5;
    		return h;
        }
        
        // floor
       	h.normal = vec3(0,-sign(endPoint.y),0);
        h.col = vec4(1,1,1, sign(endPoint.y) * clamp(sin(t*0.242)+0.8, 0.2, 1.2));
       // h.emissive = endPoint.y > 0.0;
    	return h;
    }
    return h;
}

HitTest intersectSphere(in Ray r, in Sphere s) {
	vec3 o = r.origin - s.pos;
	float v = dot(o, r.dir);
	if(v > 0.) return NOHIT;
    
	float disc = (s.radius * s.radius) - (dot(o, o) - (v * v));
	
	if(disc < 0.) return NOHIT;
	
	float dist = length(o) - (sqrt(disc));
	return HitTest(true, dist, normalize((r.origin + r.dir * dist) - s.pos), vec4(0), sin(iTime * 0.25)*.5+.5);
}

float nrand(in vec2 n) {
	return fract(sin(dot(n.xy, vec2(12.9898, 78.233)))* 43758.5453);
}

vec4 traceScene(in Camera cam, vec2 seed, float lastB) {
    vec3 startPos = cam.pos;
    
    vec4 result = vec4(0);
    
    int maxI = int(float(MAXRAYS) * lastB);
    for (int i=0; i<MAXRAYS; i++) {
        if (i==maxI) break;
    	Ray r = cam.ray;
        
        r.dir.x += (nrand(seed)*2.-1.) * divergence;
        r.dir.y += (nrand(seed.yx)*2.-1.) * divergence;
        r.dir.z += (nrand(seed.xx)*2.-1.) * divergence;
        r.dir = normalize(r.dir);
        vec4 impact = vec4(BRIGHTNESS);
        seed++;
        
        //float maxJF = float(MAXBOUNCES) * lastB;
        int maxJ = int(float(MAXBOUNCES) * lastB);
        for (int j=0; j<MAXBOUNCES; j++) {
           //	if (j==maxJ) break;
    		HitTest t0 = intersectBox(r, Box(vec3(-5,10,-5), vec3(30,20,25)));
    		HitTest t1 = intersectSphere(r, Sphere(vec3(-1,2,0), 2.0));
            t1.col = vec4(0.5, 0.6, 0.9, 0.0);
    		HitTest t2 = intersectSphere(r, Sphere(vec3(4,5,4), 5.0));
            t2.col = vec4(0.9,0.9,0,0);
            t2.ref = 0.8;
    		HitTest t3 = intersectSphere(r, Sphere(vec3(-5,4,4), 4.0));
            t3.col = vec4(0.3,0.9,0.6,sin(t * 0.6)*3.0);
            t3.ref = 0.0;
    		HitTest t4 = intersectSphere(r, Sphere(vec3(4,2.5,-2), 2.5));
            t4.col = vec4(1,0.5,0.2,sin(t * 0.7)*3.);
            t4.ref = 0.0;
    		
    		HitTest test = minT(t0, t1, t2, t3, t4);
    	
    		if (test.hit) {
        		impact *= test.col;
                if (test.col.a > 0.0) { 
        	    	result += test.col * impact * test.col.a;
        	    	//break;
        		}
                
        	    r.origin += r.dir * test.dist;
                r.origin += test.normal * 0.01;
                
                vec3 random = vec3(
        		            nrand(r.origin.xy+seed),
        		            nrand(r.origin.yz+seed),
        		            nrand(r.origin.zx+seed)
        		            )*2. - 1.;
                
                if (test != t0 && test != t2) {
                    vec3 refl = reflect(r.dir, test.normal);
                    vec3 matte = normalize(test.normal + random);
                    
                    float s = max(0.0, -dot(test.normal, r.dir));
        			s = step(pow(s, 0.5), nrand(seed));
                    r.dir = refl * s + matte * (1.-s);
                } else {
                
                	r.dir = normalize(mix(
                   	 	test.normal + random,
                    	reflect(r.dir, test.normal),
                    	test.ref
                    ));
                }
            } else {
                break;
            }
        }
    }
    return result / float(MAXRAYS);
}

// Sets up a camera at a position, pointing at a target.
// uv = fragment position (-1..1) and fov is >0 (<1 is telephoto, 1 is standard, 2 is fisheye-like)
Camera setupCam(in vec3 pos, in vec3 target, in float fov, in vec2 uv) {
		// cam setup
    vec2 mouse = iMouse.xy / iResolution.xy;
    mouse = mouse * 2. - 0.5;
    pos -= target;
    R(pos.xz, mouse.x*0.5);// + sin(t*0.05));
    R(pos.xy, mouse.y*0.5);
    pos += target;
    // Create camera at pos
	Camera cam;
    cam.pos = pos;
    
    // A ray too
    Ray ray;
    ray.origin = pos;
    
    // FOV is a simple affair...
    uv *= fov;
    
    // Now we determine hte ray direction
	vec3 cw = normalize (target - pos );
	vec3 cp = vec3 (0.0, 1.0, 0.0);
	vec3 cu = normalize ( cross(cw,cp) );
	vec3 cv = normalize ( cross (cu,cw) );
    
	ray.dir = normalize ( uv.x*cu + uv.y*cv + 0.5 *cw);
    
    // Add the ray to the camera and our work here is done.
	cam.ray = ray;
    
    // Ray divergence
    divergence = fov / iResolution.x;
    divergence = divergence + length(uv) * 0.01;
	return cam;
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    t = iTime;
	vec2 uv = fragCoord.xy / iResolution.xy;
	uv.y = 1.0 - uv.y;
    vec4 l = texture(iChannel0, uv);

    uv = (fragCoord.xy / iResolution.xy) * 2. - 1.;
    uv.y /= iResolution.x/iResolution.y;
    Camera cam = setupCam(vec3(0,3,-8), vec3(4,5,4), 1.0, uv);
        //Camera(vec3(0, 5, -10), normalize(vec3(uv, 1.0)));
    
    float lastB = max(l.x, max(l.y, l.z));
    lastB = pow(lastB, 0.25);
    vec4 c = traceScene(cam, uv + iTime, max(0.3,1.-lastB));
    fragColor = mix(c, l, iMouse.z > 0.5 || iFrame == 0 ? 0.5 : 0.98);
}