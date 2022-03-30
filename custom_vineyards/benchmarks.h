/*    This file is part of the MMA Library - https://gitlab.inria.fr/dloiseau/multipers - which is released under MIT.
 *    See file LICENSE for full license details.
 *    Author(s):       David Loiseaux
 *
 *    Copyright (C) 2021 Inria
 *
 *    Modification(s):
 *      - 2022/03 Hannah Schreiber: Integration of the new Vineyard_persistence class, renaming and cleanup.
 */
/**
 * @file benchmarks.h
 * @author David Loiseaux, Hannah Schreiber
 * @brief Functions to benchmark specific part of functions from vineyard_trajectories.h
 */

#ifndef BENCHMARKS_H_INCLUDED
#define BENCHMARKS_H_INCLUDED

#include <vector>
#include <chrono>

#include "vineyards_trajectories.h"
#include "approximation.h"
#include "images.h"
#include "utilities.h"

using Vineyard::boundary_matrix;
using Vineyard::filtration_type;
using Vineyard::point_type;
using Vineyard::corner_type;
using Vineyard::dimension_type;

double time_vineyard_barcode_computation(
        boundary_matrix& boundaryMatrix,
        const std::vector<filtration_type>& filtersList,
        double precision,
        std::pair<point_type, point_type>& box,
        bool threshold = false,
        bool multithread = false,
        const bool verbose = false)
{
	auto elapsed = clock();
    Vineyard::compute_vineyard_barcode(
                boundaryMatrix, filtersList, precision,
                box, threshold, multithread,
                verbose);
	elapsed = clock() - elapsed;
    return static_cast<float>(elapsed)/CLOCKS_PER_SEC;
}

double time_approximated_vineyard_barcode_computation(
        boundary_matrix &boundaryMatrix,
        const std::vector<filtration_type>& filtersList,
        const double precision,
        const std::pair<corner_type, corner_type>& box,
        const bool threshold = false,
        const bool keepOrder = false,
        const bool complete = true,
        const bool multithread = false,
        const bool verbose = false)
{
    if (verbose) std::cout << "Starting approx..." << std::flush;

	auto elapsed = clock();
    Vineyard::compute_vineyard_barcode_approximation(
                boundaryMatrix, filtersList, precision, box,
                threshold, keepOrder, complete, multithread, verbose
                );
	elapsed = clock() - elapsed;
    auto time = static_cast<double>(elapsed) / CLOCKS_PER_SEC;

    if (verbose) std::cout << " Done ! It took " <<  time << "seconds." << std::endl;

	return time;
}

double time_2D_image_from_boundary_matrix_construction(
        boundary_matrix &boundaryMatrix,
        const std::vector<filtration_type>& filtersList,
        const double precision,
        const std::pair<corner_type, corner_type>& box,
        const double delta,
        const std::vector<unsigned int> &resolution,
        const dimension_type dimension,
        const bool complete = true,
        const bool verbose = false)
{
	auto elapsed = clock();
    get_2D_image_from_boundary_matrix(
                boundaryMatrix, filtersList, precision, box, delta,
                resolution, dimension, complete, verbose
                );
	elapsed = clock() - elapsed;
    return static_cast<float>(elapsed)/CLOCKS_PER_SEC;
}

#endif // BENCHMARKS_H_INCLUDED
