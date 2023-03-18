#include "MysqlConn.h"
#include  "Logging.h"
MysqlConn::MysqlConn()
{
    conn_ = mysql_init(nullptr);
    //mysql_init()函数用来初始化MYSQL对象，如果成功则返回MYSQL对象的指针，如果失败则返回NULL。
    if (conn_ == nullptr)
    {
        LOG_ERROR << "mysql_init error";
    }
    mysql_set_character_set(conn_, "utf8");
}

MysqlConn::~MysqlConn()
{
    if(conn_ != nullptr)
    {
        mysql_close(conn_);
    }
    freeResult();
}

bool MysqlConn::connect(const std::string& user, const std::string& passwd, const std::string dbName, const std::string& ip, const unsigned int& port)
{
    if(conn_ == nullptr)
    {
        LOG_ERROR << "mysql_init error";
        return false;
    }
    MYSQL *p = mysql_real_connect(conn_, ip.c_str(), user.c_str(), passwd.c_str(), dbName.c_str(), port, nullptr, 0);
    if(p == nullptr)
    {
        LOG_ERROR << "mysql_real_connect error";
        return false;
    }
    return true;
}

bool MysqlConn::update(const std::string& sql)
{
    if(conn_ == nullptr)
    {
        LOG_ERROR << "mysql_init error";
        return false;
    }
    //mysql_query()函数用来执行SQL语句，如果执行成功则返回0，如果执行失败则返回非0值。
    if(mysql_query(conn_, sql.c_str()) != 0)
    {
        LOG_ERROR << "mysql_query error";
        return false;
    }
    return true;
}

bool MysqlConn::query(const std::string& sql)
{
    if(conn_ == nullptr)
    {
        LOG_ERROR << "mysql_init error";
        return false;
    }
    freeResult();
    res_ = mysql_store_result(conn_);
    //mysql_store_result()函数用来获取结果集，如果执行成功则返回结果集，如果执行失败则返回NULL。
    //结果集是一个二维表，每一行对应一条记录，每一列对应一条记录的一个字段。
    //mysql_store_result()函数只能用于SELECT语句，不能用于INSERT、UPDATE、DELETE语句。
    return true;
}

bool MysqlConn::next()
{
    if(res_ == nullptr)
    {
        LOG_ERROR << "mysql_store_result error";
        return false;
    }
    row_ = mysql_fetch_row(res_);
    //mysql_fetch_row()函数用来获取结果集中的下一行，如果成功则返回该行，如果失败则返回NULL。
    if(row_ == nullptr)
    {
        return false;
    }
    return true;
}

std::string MysqlConn::value(int index)
{
    int rowCnt = mysql_num_fields(res_);
    //mysql_num_fields()函数用来获取结果集中的字段数。
    if(index < 0 || index >= rowCnt)
    {
        LOG_ERROR << "index error";
        return "";
    }
    char *val = row_[index];
    unsigned long *lengths = mysql_fetch_lengths(res_);
    //mysql_fetch_lengths()函数用来获取结果集中每个字段的长度。
    return std::string(val, lengths[index]);
}

bool MysqlConn::transaction()
{
    
    return mysql_autocommit(conn_, 0) == 0;
    //mysql_autocommit()函数用来设置是否自动提交事务，如果设置成功则返回0，如果设置失败则返回非0值。
}

bool MysqlConn::commit()
{
    return mysql_commit(conn_) == 0;
    //mysql_commit()函数用来提交事务，如果提交成功则返回0，如果提交失败则返回非0值。
}

bool MysqlConn::rollback()
{
    return mysql_rollback(conn_) == 0;
    //mysql_rollback()函数用来回滚事务，如果回滚成功则返回0，如果回滚失败则返回非0值。
}

void MysqlConn::refreshAliveTime()
{
    lastActiveTime_ = steady_clock::now();
    //steady_clock是一个以固定时间间隔计时的时钟，这个时间间隔是不可变的，不受系统时间变化的影响。
}

long long MysqlConn::getAliveTime()
{
    std::chrono::nanoseconds ns = steady_clock::now() - lastActiveTime_;
    std::chrono::milliseconds ms = std::chrono::duration_cast<std::chrono::milliseconds>(ns);
    return ms.count();
}

void MysqlConn::freeResult()
{
    if(res_ != nullptr)
    {
        mysql_free_result(res_);
        //mysql_free_result()函数用来释放结果集的内存。
        res_ = nullptr;
    }
}


