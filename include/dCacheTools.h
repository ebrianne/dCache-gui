#ifndef DCACHETOOLS_H
#define DCACHETOOLS_H

#include <QString>
#include <QThread>
#include <QProcess>

class dCacheTools : public QThread
{
    Q_OBJECT
public:
    dCacheTools();
    ~dCacheTools();

    void run();
    void setPassword(QString pass) {_password = pass;}
    void DoList(QString dir);
    void Copy(QString Input, QString BaseDir, QString OutputDir, int type, bool isSingleFile);
    void StopCopy();
    void DestroyProxy(int timeleft);

signals:
    void log(QString type, QString message);
    void PasswordRequired();
    void ProxyStatus(QString status);
    void readyRead(QProcess *proc);
    void ProxyDestroyed();

public slots:
    void StartProxy();
    void CheckProxy();
    void readStdOut(QProcess *proc);
    void SetEnv();

private:
    QProcess *dCachetool;
    QString _password;
    QProcessEnvironment env;
};

#endif // DCACHETOOLS_H
