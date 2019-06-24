#include <filesystem>

#include "MPCAlgorithm.hpp"
#include "timing.hpp"
#include "simulation.hpp"

namespace fs = std::filesystem;

fs::path getOutputPath()
{
    return fs::path("..") / "output" / Model::getModelName();
}

int main()
{
    auto model = std::make_shared<Model>();
    MPCAlgorithm solver(model);

    double T;
    model->getTimeHorizon(T);
    const size_t sim_steps = 40;

    size_t K;
    solver.getTimesteps(K);

    double dt = T / K;

    solver.initialize();
    Model::dynamic_matrix_t X;
    Model::dynamic_matrix_t U;
    Model::dynamic_matrix_t X_sim, U_sim;
    X_sim.resize(Model::state_dim, sim_steps);
    U_sim.resize(Model::input_dim, sim_steps);

    Model::input_vector_t u_init(0, 0, -model->p.g_I.z() * model->p.m);
    Model::input_vector_t u_final = u_init;
    Model::state_vector_t x_init = model->p.x_init;

    solver.setDesiredState(model->p.x_final);
    solver.setInitialState(x_init);

    double timer_run = tic();
    for (size_t i = 0; i < sim_steps; i++)
    {
        fmt::print("\nSIMULATION STEP {}:\n", i);
        const double t0 = tic();
        solver.solve();
        solver.getSolution(X, U);
        const double solve_time = toc(t0);

        Model::state_vector_t x_sim;
        u_init = U.col(0);
        u_final = U.col(0);
        sim::simulate(*model, dt, x_init, u_init, u_final, x_sim);

        X_sim.col(i) = x_sim;
        U_sim.col(i) = U.col(0);

        u_init = u_final;
        x_init = x_sim;
        solver.setInitialState(x_init);
    }
    fmt::print("\n");
    fmt::print("{:<{}}{:.2f}ms\n", fmt::format("Time, {} steps:", sim_steps), 50, toc(timer_run));
    const double freq = double(sim_steps) / (toc(timer_run) / 1000.);
    fmt::print("{:<{}}{:.2f}Hz\n", "Average frequency:", 50, freq);
    fmt::print("\n");

    // write solution to files
    double timer = tic();
    fs::path outputPath = getOutputPath() / std::to_string(0);
    if (not fs::exists(outputPath) and not fs::create_directories(outputPath))
    {
        throw std::runtime_error("Could not create output directory!");
    }

    {
        std::ofstream f(outputPath / "X_sim.txt");
        f << X_sim;
    }
    {
        std::ofstream f(outputPath / "U_sim.txt");
        f << U_sim;
    }
    {
        std::ofstream f(outputPath / "t_sim.txt");
        f << double(sim_steps) * dt;
    }
    {
        std::ofstream f(outputPath / "X.txt");
        f << X;
    }
    {
        std::ofstream f(outputPath / "U.txt");
        f << U;
    }
    {
        std::ofstream f(outputPath / "t.txt");
        f << T;
    }
    fmt::print("{:<{}}{:.2f}ms\n", "Time, solution files:", 50, toc(timer));
}