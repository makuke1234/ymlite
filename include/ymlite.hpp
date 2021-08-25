#pragma once

#include <exception>
#include <type_traits>
#include <string>
#include <vector>
#include <unordered_map>
#include <sstream>

#include <cstdint>
#include <cstring>


namespace ymlite
{
	template<class T>
	static constexpr T const & var_max(T const & val1) noexcept
	{
		return val1;
	}
	template<class T, class ... Args>
	static constexpr T const & var_max(T const & val1, T const & val2, Args const & ... args) noexcept
	{
		return var_max((val1 < val2) ? val2 : val1, args...); 
	}


	class yaml;

	class exception : public std::exception
	{
	private:
		friend class yaml;
		enum class type : std::int_fast8_t
		{
			Unknown,
			
			ParseNoKeyValue,

		};
		static constexpr const char * exceptionMessages[]
		{
			"Unknown exception.",
			"Object key's value not found!"
		};
		type Type{ type::Unknown };
		const char * exceptionMessage{ exceptionMessages[std::underlying_type<type>::type(Type)] };

		exception() noexcept = default;
		explicit exception(const char * eMsg) noexcept
			: exceptionMessage(eMsg)
		{}

	public:
		explicit exception(type Type_) noexcept
			: Type(Type_)
		{}

		const char * what() const throw() override
		{
			return this->exceptionMessage;
		}
	};

	class yaml
	{
	public:
		// Type aliasing common types
		using charT   = char;
		using numberT = double;
		using boolT   = bool;


		using stringT = std::basic_string<charT>;
		using osstreamT = std::basic_ostringstream<charT>;
		
		template<class T>
		using vectorT = std::vector<T>;
		using arrayT  = vectorT<yaml>;
		
		template<class T>
		using objmapT = std::unordered_map<stringT, T>;
		using objectT = objmapT<yaml>;

	private:
		static constexpr auto TypeUnion_sz = var_max(
			sizeof(stringT), sizeof(numberT), sizeof(boolT),
			sizeof(vectorT<void *>), sizeof(objmapT<void *>)
		);
		union TypeUnion
		{
			std::uint8_t bytes[TypeUnion_sz];
		};
		enum class datatype : std::int8_t
		{
			Null,
			String,
			Number,
			Boolean,
			Array,
			Object
		};

		TypeUnion ndata;
		datatype role { datatype::Null };

		template<class ... Args>
		inline void make_String(Args & ... args)
		{
			new (&this->ndata) stringT(std::forward<Args>(args)...);
			role = datatype::String;
		}
		inline void make_Number(numberT value)
		{
			new (&this->ndata) numberT{ value };
			role = datatype::Number;
		}
		inline void make_Bool(boolT value)
		{
			new (&this->ndata) boolT{ value };
			role = datatype::Boolean;
		}
		template<class ... Args>
		inline void make_Array(Args & ... args)
		{
			new (&this->ndata) arrayT(std::forward<Args>(args)...);
			role = datatype::Array;
		}
		template<class ... Args>
		inline void make_Object(Args & ... args)
		{
			new (&this->ndata) objectT(std::forward<Args>(args)...);
			role = datatype::Object;
		}

		inline void destroy_String() noexcept
		{
			static_cast<stringT *>(static_cast<void *>(&this->ndata))->~basic_string();
			role = datatype::Null;
		}
		inline void destroy_Number_or_Boolean() noexcept
		{
			role = datatype::Null;
		}
		inline void destroy_Array() noexcept
		{
			static_cast<arrayT *>(static_cast<void *>(&this->ndata))->~vector();
			role = datatype::Null;
		}
		inline void destroy_Object() noexcept
		{
			static_cast<objectT *>(static_cast<void *>(&this->ndata))->~unordered_map();
			role = datatype::Null;
		}

		inline stringT & get_String() noexcept
		{
			return *static_cast<stringT *>(static_cast<void *>(&this->ndata));
		}
		inline stringT const & get_cString() const noexcept
		{
			return *static_cast<const stringT *>(static_cast<const void *>(&this->ndata));
		}
		inline numberT & get_Number() noexcept
		{
			return *static_cast<numberT *>(static_cast<void *>(&this->ndata));
		}
		inline numberT const & get_cNumber() const noexcept
		{
			return *static_cast<const numberT *>(static_cast<const void *>(&this->ndata));
		}
		inline boolT & get_Bool() noexcept
		{
			return *static_cast<boolT *>(static_cast<void *>(&this->ndata));
		}
		inline boolT const & get_cBool() const noexcept
		{
			return *static_cast<const boolT *>(static_cast<const void *>(&this->ndata));
		}
		inline arrayT & get_Array() noexcept
		{
			return *static_cast<arrayT *>(static_cast<void *>(&this->ndata));
		}
		inline arrayT const & get_cArray() const noexcept
		{
			return *static_cast<const arrayT *>(static_cast<const void *>(&this->ndata));
		}
		inline objectT & get_Object() noexcept
		{
			return *static_cast<objectT *>(static_cast<void *>(&this->ndata));
		}
		inline objectT const & get_cObject() const noexcept
		{
			return *static_cast<const objectT *>(static_cast<const void *>(&this->ndata));
		}


		
		static inline yaml p_parse(const charT * str, std::size_t len);
		static inline stringT p_dump(yaml const & instance, std::size_t depth);

	public:
		static inline yaml parse(const charT * str, std::size_t len = 0)
		{
			// If length not give, calc
			if (len == 0)
				len = std::char_traits<charT>::length(str);
			// Calling another private recursive function to avoid length-checking
			return yaml::p_parse(str, len);
		}
		static inline yaml parse(stringT const & str)
		{
			return yaml::p_parse(str.c_str(), str.length());
		}
		inline stringT dump() const
		{
			return this->p_dump(*this, std::size_t(1));
		}

		yaml() noexcept = default;
		yaml(yaml const & other);
		yaml(yaml && other) noexcept;
		yaml & operator=(yaml const & other);
		yaml & operator=(yaml && other) noexcept;
		~yaml() noexcept;

		yaml & operator[](const char * key_);
		yaml const & operator[](const char * key_) const;
		yaml & operator[](std::string const & key_);
		yaml const & operator[](std::string const & key_) const;

		std::string & get();
		std::string const & get() const;

	};

	inline yaml yaml::p_parse(const yaml::charT * str, std::size_t len)
	{
		return {};
	}
	inline yaml::stringT yaml::p_dump(yaml const & inst, std::size_t depth)
	{
		osstreamT dumpstream;

		return dumpstream.str();
	}

	yaml::yaml(yaml const & other)
		: role(other.role)
	{
		switch (this->role)
		{
		case datatype::String:
			this->make_String(other.get_cString());
			break;
		case datatype::Number:
			this->get_Number() = other.get_cNumber();
			break;
		case datatype::Boolean:
			this->get_Bool() = other.get_cBool();
			break;
		case datatype::Array:
			this->make_Array(other.get_cArray());
			break;
		case datatype::Object:
			this->make_Object(other.get_cObject());
			break;
		}
	}
	yaml::yaml(yaml && other) noexcept
		: role(other.role)
	{
		std::memcpy(&this->ndata, &other.ndata, sizeof this->ndata);
		std::memset(&other.ndata, 0, sizeof other.ndata);
		other.role = datatype::Null;
	}
	yaml & yaml::operator=(yaml const & other)
	{
		this->~yaml();
		this->role = other.role;
		switch (this->role)
		{
		case datatype::String:
			this->make_String(other.get_cString());
			break;
		case datatype::Number:
			this->get_Number() = other.get_cNumber();
			break;
		case datatype::Boolean:
			this->get_Bool() = other.get_cBool();
			break;
		case datatype::Array:
			this->make_Array(other.get_cArray());
			break;
		case datatype::Object:
			this->make_Object(other.get_cObject());
			break;
		}
		return *this;
	}
	yaml & yaml::operator=(yaml && other) noexcept
	{
		this->~yaml();
		this->role = other.role;
		other.role = datatype::Null;
		std::memcpy(&this->ndata, &other.ndata, sizeof this->ndata);
		std::memset(&other.ndata, 0, sizeof other.ndata);
		return *this;
	}
	yaml::~yaml() noexcept
	{
		switch (this->role)
		{
		case datatype::String:
			this->destroy_String();
			break;
		case datatype::Number:
		case datatype::Boolean:
			this->destroy_Number_or_Boolean();
			break;
		case datatype::Array:
			this->destroy_Array();
			break;
		case datatype::Object:
			this->destroy_Object();
			break;
		}
	}
}
