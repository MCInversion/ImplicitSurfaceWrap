#pragma once

namespace Utils
{
    /// ==================================================================
    /// \brief Interface for file mapping wrapper.
    /// \class IFileMappingWrapper
    /// ================================================================== 
    class IFileMappingWrapper
    {
    public:
        virtual ~IFileMappingWrapper() = default;

       virtual [[nodiscard]] const void* GetFileMemory() const = 0;
       [[nodiscard]] virtual size_t GetFileSize() const = 0;
    };

} // namespace Utils