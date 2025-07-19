#pragma once

#include <string>
#include <vector>
#include <memory>
#include <map>
#include <variant>
#include <cstdint>
#include <atomic>>

// Forward declaration
struct sqlite3;
struct sqlite3_stmt;

namespace E5D {
namespace Studio {

    // 数据类型定义
    using DataValue = std::variant<std::string, int, double, bool, std::nullptr_t>;
    using DataRow = std::map<std::string, DataValue>;
    using DataTable = std::vector<DataRow>;

    // 材质类定义
    class BaseMaterial {
    public:
        uint32_t id = 0;
        float diffuse[3] = {0.0f, 0.0f, 0.0f};
        float dissolve = 0.0f;  // 1 = 全透明
        float roughness = 0.0f;
        float metallic = 0.0f;
        std::string diffuse_texname;
        std::string alpha_texname;
        std::string normal_texname;
        std::string metallic_texname;
        std::string roughness_texname;
    };

    enum class SQLiteSynchronousType
    {
        FULL,
        NORMAL,
        OFF,
    };

    class DataAccess {
    public:
        DataAccess();
        ~DataAccess();

        // 常量定义
        static const int64_t ModelClassId = 1;
        static const int64_t ComponentClassId = 2;
        static const int64_t ShapeClassId = 3;
        static const int64_t TreeAssoId = 1;

        // 数据库连接管理
        bool OpenLocalDatabase();
        bool CloseLocalDatabase();
        bool QuietCloseLocalDatabase();
        bool IsConnected() const;

    protected:
        // 基本SQL操作
        bool ExecuteNonQuery(const std::string& sql);
        DataTable ExecuteQuery(const std::string& sql);
        
        // 参数化查询
        bool ExecuteNonQuery(const std::string& sql, const std::vector<DataValue>& parameters);
        DataTable ExecuteQuery(const std::string& sql, const std::vector<DataValue>& parameters);

        
        // 事务管理
        bool BeginTransaction();
        bool CommitTransaction();
        bool RollbackTransaction();

    public:

        // 批处理事务管理
        bool BeginForBatch();
        bool CommitForBatch(bool commit = true);
        bool EndForBatch(bool commit = true);

        // 数据库设置
        bool EnableWAL();
        bool SetCacheSize(const int64_t& cache_size);
        bool SetMMapSize(const int64_t& mmap_size);

        //bool SetPageSize(const int& page_size);

        bool SetWalAutoCheckpoint(const int& autocheckpoint);

        bool SetJournalSizeLimit(const int& walfilesize);

        bool SetSynchronous(const SQLiteSynchronousType& Synchronous);

        bool DeleteIndexBeforeBatch();

        bool ReIndexAfterBatch();

        // 数据操作方法
        bool GetMaxId(const std::string& tabname, int64_t& maxid);
        bool AddInstance(const int64_t& id, const int64_t& classid, const std::string& name, const char* path, const bool& isSignificant);
        bool AddInstance(const int64_t& id, const int64_t& classid, const std::string& name, const char* path, const bool& isSignificant,
            const double& min_x, const double& min_y, const double& min_z, const double& max_x, const double& max_y, const double& max_z);
        bool AddMaterial(const int64_t& id, const std::string& textureFolder, const BaseMaterial& material);
        bool AddModel(const std::string& modelName, const int64_t& id);
        bool AddModel(const std::string& modelName, const int64_t& id, 
            const double& min_x, const double& min_y, const double& min_z, const double& max_x, const double& max_y, const double& max_z);
        bool AddAssoType(const std::string& name, const std::string& desc, const std::string& source_role, const std::string& target_role, const int& id);
        bool AddAsso(const int64_t& parentId, const int64_t& id, const int64_t& assoType);
        bool AddShape(const int64_t& id, const int64_t& instid, const int64_t& geo_id, const int& material_id, const double& min_x, const double& min_y, const double& min_z, const double& max_x, const double& max_y, const double& max_z, const std::string& matrix);
        bool AddGeometry(const int64_t& id, const int64_t& hashcode, int geo_type, const std::vector<uint8_t>& geo_byte);
        bool AddMesh(const int64_t& id, const int64_t& geo_id, int mesh_tri_count, const double& xmin, const double& ymin, const double& zmin, const double& xmax, const double& ymax, const double& zmax, const std::vector<uint8_t>& mesh);
        bool AddMesh(const int64_t& id, const int64_t& geo_id, int mesh_tri_count, const double& xmin, const double& ymin, const double& zmin, const double& xmax, const double& ymax, const double& zmax, const std::string& mesh);
        bool UpdateInstanceBoundingBox(const int64_t& instid, const double& min_x, const double& min_y, const double& min_z, const double& max_x, const double& max_y, const double& max_z);
        bool UpdateProjectSettings(const std::map<std::string, std::string>& settings);

        // 实用方法
        std::string GetLastError() const;
        int GetLastInsertRowId() const;
        int GetChangedRowCount() const;

        // 数据库路径
        std::string E5dDbPath;

        int AutoCommitNum = 5000;
        std::atomic<int> PendingCommitBatchNum = 0;

    private:
        sqlite3* m_database;
        std::string m_lastError;
        bool m_isCompressedDB;
        bool m_transactioning;

        // 预编译语句
        sqlite3_stmt* m_addModelCmd;
        sqlite3_stmt* m_addModelAndBoundBoxCmd;
        sqlite3_stmt* m_addInstanceCmd;
        sqlite3_stmt* m_addInstanceAndBoundBoxCmd;
        sqlite3_stmt* m_addMaterialCmd;
        sqlite3_stmt* m_addAssoCmd;
        sqlite3_stmt* m_addShapeCmd;
        sqlite3_stmt* m_addGeometryCmd;
        sqlite3_stmt* m_addMeshCmd;
        sqlite3_stmt* m_updateInstanceBoundingBoxCmd;
        
        // 内部辅助方法
        int ExecuteSQL(const std::string& sql);
        bool Transaction();
        bool Commit();
        bool RollBack();
        bool PrepareStatement(const std::string& sql, sqlite3_stmt** stmt);
        void BindParameters(sqlite3_stmt* stmt, const std::vector<DataValue>& parameters);
        DataValue GetColumnValue(sqlite3_stmt* stmt, int columnIndex);
        void SetLastError(const std::string& error);
        void ClearError();
        void CleanupStatements();
        std::vector<uint8_t> ReadFileToBytes(const std::string& filePath);

        void AddAndCheckPenddingCommit();
    };

    // 工具类
    class DataAccessUtil {
    public:
        static std::string EnsureDatabase(const std::string& output, const std::string& dbName, const std::string& templateName);
    };

} // namespace Studio
} // namespace E5D