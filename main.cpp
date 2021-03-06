#include <iostream>
#include <ctime>
#include <fenv.h>
#include <fstream>
#include <iomanip>
#include <cmath>
#include <memory.h>
#include <cstdlib>
#include <cstdio>
#include <sstream>
#include <omp.h>
#include <iomanip>
#include "vec3.h"
#include <vector>

using namespace std;

typedef double (*force_type)(double);
typedef double (*potential_type)(double);
typedef Vec3 (*alg_V)(Vec3, double);
typedef Vec3 (*alg_R)(Vec3, Vec3, Vec3, double, double);

#define POTENTIAL_ENERGY_PERCENT 0.01
#define ENERGY_PERCENT 0.1

const double eps = 1;
const double sigma = 1;
//const double rcut = 2.5 * sigma;
const double rmin = 0.00001;

double force_LD(double r)
{
//    if(r > rcut)
//        return 0;

    if(r < rmin)
        return force_LD(rmin);

    double x = sigma / r;
    return - (48 * (eps / sigma) * (pow(x, 13) - 0.5 * pow(x, 7)));
}

double potential_LD(double r)
{
//    if(r > rcut)
//        return 0;

    if(r < rmin)
        return potential_LD(rmin);

    double x = sigma / r;
    return 4 * eps * (pow(x, 12) - pow(x, 6));
}

Vec3 Verle_R(Vec3 r, Vec3 dr, Vec3 f, double m, double dt)
{
    Vec3 tmp = (f / (2 * m)) * dt * dt;
    return r + dr + tmp;
}

Vec3 Verle_V (Vec3 dr, double dt)
{
    return dr / (2 * dt);
}

class physical_system
{
    double L_FREE_MOTION; //для проверки что r(t + dt) - r(t) не слишком велико
    const unsigned long N = 50; //количество частиц
    const double Lx = 800, Ly = 800, Lz = 800; //размер ячейки в 10е-11 м
    const double dt = 0.001; //время 1 молекулярного-шага в 10е-12 сек
    const double T0 = 1.38 * 110; //начальная температура
    bool v0_rand = true; //наличие начальных скоростей у частиц
    bool isInitialised = false; //инициализирована система
    vector<Vec3> r; //соординаты всех частиц
    Vec3 * dr; //расстояния от предыдущих координат системы до текущих
    Vec3 * v; //скорости всех частиц
    Vec3 * f; //силы действующие на частицу со стороны остальных частиц
    double * m; //массы всех частицы
    double energy; //полная энергия системы
    double initial_energy = 0; // начальная энергия системы
    double potential_energy;
    double kinetic_energy;
    double averV; //средняя скорость чатсиц системы
    double sqraverV; //средняя квадратичная скорость чатсиц системы
    double T; //текущая температура системы
    double msys = 0; //полная масса системы
    Vec3 Psys; //импульс системы
    force_type force;
    alg_R calc_R;
    alg_V calc_V;
    potential_type potential;
    double time; //текущее время
    double termalization_time = 5; //время термализации в 10е-12 сек
public:
    ofstream out;
    ofstream term_out;
    ofstream force_out;
    ofstream temperature_out, full_energy_out;
    physical_system(force_type Force, alg_R Calc_R, alg_V Calc_v,
                    potential_type Potential) : force(Force), calc_R(Calc_R), calc_V(Calc_v), potential(Potential)
    {
        dr = new Vec3[N];
        v = new Vec3[N];
        f = new Vec3[N];
        r.resize(N, Vec3(0.0, 0.0, 0.0));
        m = new double[N];
        time = 0.0;
        energy = 0.0;
        kinetic_energy = 0.0;
        initial_energy = 0.0;
        out.open("output.dat");
        term_out.open("output.dat");
        force_out.open("forces.dat");
        temperature_out.open("temperature.dat");
        full_energy_out.open("energy.dat");
        L_FREE_MOTION = (pow(Lx * Ly * Lz, 1.0/3) / (2*N));
    }

    ~physical_system()
    {
        delete [] dr;
        delete [] v;
        delete [] f;
        delete [] m;
        r.clear();
        out.close();
        term_out.close();
        force_out.close();
        temperature_out.close();
        full_energy_out.close();
    }

    void new_frame()
    {
        out << N << endl;
        out << "***** time = " << time << " ***** " << endl;
        if(time > termalization_time)
        {
            term_out << N << endl;
            term_out << "***** time = " << time << " ***** " << endl;
        }
    }

    void print_to_file(unsigned long i)
    {
        /*
         * запись в файл информации об iтой частице
         * */
        out << "Ar" << setw(10) << r[i].getX() << " " << setw(10)
            << r[i].getY() << " " << setw(10) << r[i].getZ() << " " << setw(10)
            << v[i].getX() << " " << setw(10) << v[i].getY() << " " << setw(10)
            << v[i].getZ() << endl;
        if(time > termalization_time)
        {
            term_out << "Ar" << setw(10) << r[i].getX() << " " << setw(10)
                << r[i].getY() << " " << setw(10) << r[i].getZ() << " " << setw(10)
                << v[i].getX() << " " << setw(10) << v[i].getY() << " " << setw(10)
                << v[i].getZ() << endl;
        }
    }

    void initialize()
    {
        srand(static_cast<unsigned int>(std::time(nullptr)));
        unsigned long K = static_cast<unsigned long>(ceil(pow(N, 1.0/3)));
        double dLx = Lx / (K);
        double dLy = Ly / (K);
        double dLz = Lz / (K);
        unsigned long counter = 0;
        new_frame();
        cout << "Number of particles N: " << N << endl;
        cout << "Box size: Lx = " << Lx << " Ly = " << Ly << " Lz = " << Lz << endl;

        if(v0_rand)
        {
            /*
             * Если мы хотим чтобы были начальный скорости у частиц
             */
            for(unsigned long i = 0; i < N; ++i)
            {
                v[i] = Vec3((rand() % 10000 - 5000) / 10000.0,
                            (rand() % 10000 - 5000) / 10000.0,
                            (rand() % 10000 - 5000) / 10000.0);
                m[i] = 1; //m_Ar = 6,6e-26 kg
            }
            calc_stats();
            cout << "init T = " << T << endl;
            for(unsigned long i = 0; i < N; ++i)
            {
                /*
                 * масштабирование скоростей
                 */
                v[i] = Vec3(v[i].getX() * pow(T0 / T, 0.5),
                            v[i].getY() * pow(T0 / T, 0.5),
                            v[i].getZ() * pow(T0 / T, 0.5));
            }

            calc_P();

            for(unsigned long i = 0; i < N; ++i)
            {
                /*
                 * вычтем скорость центра масс системы чтобы она покоилась
                 */
                v[i] -= Psys * (1 / msys);
            }
        }
        else
        {
            for(unsigned long i = 0; i < N; ++i)
            {
                /*
                 * если мы хотим чтобы частицы покоились
                 */
                v[i] = Vec3(0.0, 0.0, 0.0);
                m[i] = 1;
            }
        }
        for(unsigned long i = 0; i < K; ++i)
            for(unsigned long j = 0; j < K; ++j)
                for(unsigned long k = 0; k < K; ++k)
                    if(counter < N)
                    {
                        r[counter].set((i + 1.0/2) * dLx, (j + 1.0/2) * dLy, (k + 1.0/2) * dLz);
                        dr[counter].set(v[counter].getX() * 2 * dt,
                                        v[counter].getY() * 2 * dt,
                                        v[counter].getZ() * 2 * dt);
                        if((dr[counter].abs() > L_FREE_MOTION))
                        {
                            cout << "dr[counter].abs() > L_FREE_MOTION" << endl << dr[counter].abs() << " " << L_FREE_MOTION << endl;
                            exit(-1);
                        }

                        print_to_file(counter);
                        ++counter;
                    }
        isInitialised = true;
        if(!isInitialised)
        {
            cout << "Something went wrong! System is not initialized!" << endl;
            exit(-1);
        }
        calc_stats();
        initial_energy = energy;
        cout << "Potential energy = " << potential_energy << endl;
        cout << "Kinetic energy = " << kinetic_energy << endl;
        cout << "Energy = " << energy << endl;
//        if(!v0_rand)
//        {
//            cout << "!v0_rand" << endl;
//            exit(-1);
//        }
        if(fabs(potential_energy / initial_energy) > POTENTIAL_ENERGY_PERCENT)
        {
            cout << "fabs(potential_energy / initial_energy) > POTENTIAL_ENERGY_PERCENT" << endl << fabs(potential_energy / initial_energy) << " " << POTENTIAL_ENERGY_PERCENT << endl;
            exit(-1);
        }
        cout << "Initial energy = " << initial_energy << endl;
    }

    void calc_P()
    {
        /*
         * вычисление импульса центра масс системы
         */
        Psys = Vec3(0.0, 0.0, 0.0);
        if(msys == 0.0)
        {
            for(unsigned long i = 0; i < N; ++i)
            {
                Psys += v[i] * m[i];
                msys += m[i];
            }
        }
        else
        {
            for(unsigned long i = 0; i < N; ++i)
                Psys += v[i]*m[i];
        }
    }

    Vec3 NIM(Vec3 r1, Vec3 r2, double Dx, double Dy, double Dz) const
    {
        /*
         * метод ближайшего образа
         */
        double X = -(r1.getX() - r2.getX());
        double Y = -(r1.getY() - r2.getY());
        double Z = -(r1.getZ() - r2.getZ());

        Vec3 dist = Vec3(0.0, 0.0, 0.0);

        dist.set(NIM_fix(X, Dx), NIM_fix(Y, Dy), NIM_fix(Z, Dz));

        if(dist.abs() > Vec3(Dx, Dy, Dz).abs())
        {
            cout << "dist.abs() > Vec3(Dx, Dy, Dz).abs()" << dist.abs() << " " << Vec3(Dx, Dy, Dz).abs() << endl;
            exit(-1);
        }

        return (dist);
    }

    double NIM_fix(double coord, double l) const
    {
        if(coord >= l / 2.0)
        {
            coord = l - coord;
        }
        else if(coord <= -l/2.0)
        {
            coord += l;
        }

        return coord;
    }

    void PBC(Vec3 & dist, double Dx, double Dy, double Dz) const
    {
        /*
         * периодические граничные условия
         */
        double aDx = 0.0;
        double aDy = 0.0;
        double aDz = 0.0;

        dist.setX(correctCoord(dist.getX(), aDx, Dx));
        dist.setY(correctCoord(dist.getY(), aDy, Dy));
        dist.setZ(correctCoord(dist.getZ(), aDz, Dz));
    }

    double correctCoord(double coord, double leftBounadary, double rightBoundary) const
    {
        double l = rightBoundary - leftBounadary;
        double d;

        if(coord >= rightBoundary)
        {
            d = coord - leftBounadary;
            coord = (coord - l * floor(d / l));
        }

        else if(coord < leftBounadary)
        {
            d = leftBounadary - coord;
            coord = (rightBoundary - l * ((d / l) - floor(d / l)));
        }

        return coord;
    }

    void calc_forces()
    {
        double ff, _rij;
//#pragma omp parallel for private(ff, _rij)
        for(unsigned long i = 0; i < N; ++i)
        {
            f[i] = Vec3(0.0, 0.0, 0.0);
            for(unsigned long j = 0; j < N; ++j)
            {
                if(i != j)
                {
                    Vec3 rij = NIM(r[i], r[j], Lx, Ly, Lz);
                    _rij = rij.abs();
                    force_out << "_rij = " << _rij << "; i = " << i << " j = " << j << endl;
                    ff = force(_rij);
                    force_out << "force = " << ff << "; ";
                    Vec3 dr = rij;
                    force_out << "dr = " << dr << endl;
                    dr.normalize();
                    force_out << "dr.normalize = " << dr << endl;
                    Vec3 tmp;
                    tmp = dr * ff;
                    force_out << "tmp = " << tmp << endl;
                    f[i] += tmp;
                    force_out << "f[" << i << "] = " << f[i] << endl << endl;
                }
            }
        }
    }
    void integrate()
    {
        /*
         * функция для интегрирования
         * уравнений движения
         */
        Vec3 rBuf;
        time += dt;
        new_frame();
//#pragma omp parallel for private(rBuf)
        for(unsigned long i = 0; i < N; ++i)
        {
            rBuf = r[i];
            /*
             * Вычисление координат используя алгоритм Верле
             */
            r[i] = calc_R(r[i], dr[i], f[i], m[i], dt);
            dr[i] = r[i] - rBuf;
            /*
             * Проверка что r(t+dt) - r(t) не слишком велико
             */
            if(dr[i].abs() > L_FREE_MOTION)
            {
                cout << "dr[i].abs() > L_FREE_MOTION" << endl << dr[i].abs() << " " << L_FREE_MOTION << endl;
                exit(-1);
            }
            /*
             * Вычисление координат используя алгоритм Верле
             */
            v[i] = calc_V(dr[i], dt);
            /*
             * Использование периодических граничных условий
             */
            PBC(r[i], Lx, Ly, Lz);
        }

        for(unsigned long i = 0; i < N; ++i)
        {
            print_to_file(i);
        }
        calc_stats();
        temperature_out << time << "  " << T << endl;
        full_energy_out << time << "  " << energy << endl;
    }

    void calc_stats()
    {
        /*
         * Вычисление некоторых физических величин, таких как средня скорость и т.п.
         */
        double avgV = 0.0;
        double sqrAvgV = 0;
        double kinEnergy = 0.0;
        double potEnergy = 0.0;
        double E = 0.0;
        //const double cor_ratio = sqrt(3 * M_PI / 8);
        double ratio = 0;
//#pragma omp parallel for reduction (+ : avgV, sqrAvgV, kinEnergy, potEnergy)
        for (unsigned long i = 0; i < N; ++ i)
        {
            if(isInitialised)
            {
                avgV += v[i].abs();
                sqrAvgV += v[i].abs2();
                kinEnergy += m[i] * v[i].abs2() / 2;
                for(unsigned long j = 0; j < N; ++j)
                {
                    if(i != j)
                    {
                        Vec3 dr = NIM(r[i], r[j], Lx, Ly, Lz);
                        double rij = dr.abs();
                        potEnergy += potential(rij);
                    }
                }
            }
            else
            {
                kinEnergy += m[i] * v[i].abs2() / 2;
            }
        }
        avgV /= N;
        sqrAvgV = sqrt(avgV / N);
        potEnergy /= 2;
        T = (2.0 * kinEnergy) / 3.0;
        E = potEnergy + kinEnergy;
        /*
         * Сообщает что моделирование происходит корректно
         */
        if(isInitialised)
        {
            ratio = sqrAvgV / avgV;
//            if((fabs(ratio - cor_ratio) / cor_ratio) > 0.05)
//            {
//                cout << "(fabs(ratio - cor_ratio) / cor_ratio) > 0.05" << endl << ((fabs(ratio - cor_ratio) / cor_ratio)) << endl;
//                exit(-1);
//            }
            if(initial_energy != 0.0)
            {
                if(fabs((E - initial_energy) / initial_energy) > ENERGY_PERCENT)
                {
                    cout << "fabs((E - initial_energy) / initial_energy) > ENERGY_PERCENT" << endl << fabs((E - initial_energy) / initial_energy) << endl << ENERGY_PERCENT << endl;
                    exit(-1);
                }
            }
        }
        averV = avgV;
        sqraverV = sqrAvgV;
        kinetic_energy = kinEnergy;
        potential_energy = potEnergy;
        energy = E;
    }

    double calc_max_probab_v() const
    {
        const int intervals = 30;
        double max_v = 0.0;

        int * v_list = new int[intervals];
        memset(v_list, 0, sizeof (int) * intervals);
        double tmp_v;

        for(unsigned long i = 0; i < N; ++i)
        {
            tmp_v = v[i].abs();
            if(max_v < tmp_v)
                max_v = tmp_v;
        }

        double delta = max_v / intervals;

        int max_group = 0;
        int max_group_prob = 0;
        int _2_nd_max_group = 0;
        int _2_nd_max_group_prob = 0;
        for(unsigned long i = 0; i < N; ++i)
        {
            tmp_v = v[i].abs();
            int group_no = static_cast<int>(floor(tmp_v / delta));
            group_no = (group_no < intervals) ? group_no : (group_no - 1);

            v_list[group_no] ++;

            if(v_list[group_no] > max_group_prob)
            {
                max_group = group_no;
                max_group_prob = v_list[group_no];
            }
            else if(v_list[group_no] > _2_nd_max_group)
            {
                _2_nd_max_group = group_no;
                _2_nd_max_group_prob = v_list[group_no];
            }
        }

        cout << "max_v = " << max_v << endl;
        cout << "step_v = " << delta << endl;
        cout << "max_prob_v N = " << v_list[max_group] << endl;
        cout << "2nd_max_prob_v N = " << v_list[_2_nd_max_group] << endl;

        delete [] v_list;

        return (max_group + 0.5) * delta;
    }

    void measure_all()
    {
        /*
         * Функция для подсчёта всех переменных которые мы хотим знать
         */
        calc_stats();
        double max_prob_v = calc_max_probab_v();
        cout << "prob_v = " << max_prob_v << endl;
        cout << "avg_v = " << averV << endl;
        calc_P();
        cout << "Mass center impulse = " << Psys.abs() << endl;
        for(unsigned long i = 0; i < N; ++i)
        {
            v[i] -= Psys * (1/msys);
        }
        cout << "kinetic energy = " << kinetic_energy << endl;
        cout << "full energy = " << energy << endl;
    }

    void output_data()
    {
        cout << "Modeling is finished, measuring final data" << endl;
        cout << "************************* " << endl;
        measure_all();
        cout << "Measuring current temperature " << endl;
        cout << "Final T = " << T << " vs " << "Initial T = " << T0 << endl;
        cout << "*************************" << endl;
    }
};

int main()
{
    //    feenableexcept(FE_ALL_EXCEPT & -FE_INEXACT);
        //omp_set_num_threads(omp_get_max_threads());
        physical_system sample(force_LD, Verle_R, Verle_V, potential_LD);

        cout << "**********************" << endl;
        cout << "* Molecular Dynamics *" << endl;
        cout << "*   Sample program   *" << endl;
        cout << "*                    *" << endl;
        cout << "*      v. 1.0.0      *" << endl;
        cout << "**********************" << endl;

        int number_of_steps = 8000;
        cout << "Carrying out experiment..." << endl;
        sample.initialize();
        int time_for_kinE_count = static_cast<int>(0.1 * number_of_steps);
        int counter_for_kinE = 0;
        double start = static_cast<double>(time(nullptr));
        for(int i = 0; i < number_of_steps; ++i)
        {
            sample.calc_forces();
            sample.integrate();
            if(counter_for_kinE == time_for_kinE_count)
            {
                cout << "MD step #" << i << ", time = " << (static_cast<double>(time(nullptr)) - start) << endl;
                sample.measure_all();
                counter_for_kinE = 0;
                start = static_cast<double>(time(nullptr));
            }
            else
            {
                ++ counter_for_kinE;
            }
        }
        sample.output_data();

        return 0;
}
