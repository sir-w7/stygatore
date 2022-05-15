@output vector.h

@template Vec4 <- T {
	typedef struct @template_name @template_name;
	struct @template_name
	{
		T x;
		T y;
		T z;
		T w;
	};
}

@template Vec3 <- T {
	typedef struct @template_name @template_name;
	struct @template_name
	{
		T x;
		T y;
		T z;
	};
}

@template Vec2 <- T {
	typedef struct @template_name @template_name;
	struct @template_name
	{
		T x;
		T y;
	};
}

@Vec4 -> f32 -> Vec4f
@Vec4 -> u8 -> Vec4c

@Vec3 -> f32 -> Vec3f
@Vec3 -> u8 -> Vec3c

@Vec2 -> f32 -> Vec2f
@Vec2 -> u16 -> Vec2Coords
