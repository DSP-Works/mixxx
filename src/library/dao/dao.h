#ifndef DAO_H
#define DAO_H

class DAO {
  public:
    virtual ~DAO() {}

    virtual void initialize(const QSqlDatabase& database) = 0;
};

#endif /* DAO_H */
