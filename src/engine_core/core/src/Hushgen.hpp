
#ifdef HUSH_GENERATED_BODY
#undef HUSH_GENERATED_BODY
#endif

#define HUSH_GENERATED_BODY                                                                                            \
public:                                                                                                                \
	static constexpr std::uint64_t TypeId()                                                                            \
	{                                                                                                                  \
		return 0;                                                                                                      \
	}                                                                                                                  \
	static constexpr std::string_view TypeName()                                                                       \
	{                                                                                                                  \
		return "";                                                                                                     \
	}                                                                                                                  \
	static void RegisterReflection(Hush::Reflection::ReflectionDB &db)                                                 \
	{                                                                                                                  \
		(void)db;                                                                                                      \
	}                                                                                                                  \
                                                                                                                       \
	template <typename T>                                                                                              \
	Hush::Serialization::ESerializationError Serialize(T &serializer) const                                            \
	{                                                                                                                  \
		(void)serializer;                                                                                              \
		return Hush::Serialization::ESerializationError::InvalidType;                                                  \
	}                                                                                                                  \
	auto Deserialize(Hush::Serialization::IVisitor *parent, Hush::Serialization::EFormatDescribingType format)         \
	{                                                                                                                  \
		struct Visitor : public Hush::Serialization::IVisitor                                                          \
		{                                                                                                              \
			explicit Visitor(IVisitor *parent, Hush::Serialization::EFormatDescribingType format)                      \
				: IVisitor(parent, format)                                                                             \
			{                                                                                                          \
				(void)format;                                                                                          \
			}                                                                                                          \
		};                                                                                                             \
                                                                                                                       \
		return Visitor{parent, format};                                                                                \
	}                                                                                                                  \
                                                                                                                       \
private:
