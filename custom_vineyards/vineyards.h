/*    This file is part of the MMA Library - https://gitlab.inria.fr/dloiseau/multipers - which is released under MIT.
 *    See file LICENSE for full license details.
 *    Author(s):       Mathieu Carrière, David Loiseaux
 *
 *    Copyright (C) 2021 Inria
 *
 *    Modification(s):
 *      - 2022/03 Hannah Schreiber: Rewriting of the Vineyard_persistence class with new matrix data structure.
 */
/**
 * @file vineyards.h
 * @author Mathieu Carrière, David Loiseaux, Hannah Schreiber
 * @brief Core vineyards functions.
 */

#ifndef VINEYARDS_H_INCLUDED
#define VINEYARDS_H_INCLUDED

#include <vector>
#include <iostream>
#include <limits>
#include <unordered_map>

#include "debug.h"
#include "combinatory.h"

#include "utilities.h"
//#include "ru_matrix.h"
//#include "list_column.h"

namespace Vineyard {

struct Filtration_creator
{
    //assumes matrix in an valid filtration order and the all vertices
    //are in the first cells of vertexOrder in the same order than in matrix
    static void get_lower_star_filtration(
            const boundary_matrix& matrix,
            const std::vector<filtration_value_type>& vertexOrder,
            filtration_type& resultingFilter)
    {
        resultingFilter.resize(matrix.size());
        int currentVertexPosition = 0;

        for (unsigned int i = 0; i < matrix.size(); i++)
        {
            const boundary_type& b = matrix.at(i);
            if (b.size() == 0){
                resultingFilter.at(i) = vertexOrder.at(currentVertexPosition++);
            } else {
                resultingFilter.at(i) = resultingFilter.at(b.front());
                for (unsigned int j = 1; j < b.size(); j++)
                    resultingFilter.at(i) = std::max(resultingFilter.at(i),
                                                     resultingFilter.at(b.at(j)));
            }
        }
    }
};

template<class Vineyard_matrix_type/* = RU_matrix<List_column>*/>
/**
 * @brief Stores the variables to be able to apply a vineyard update
 * 
 */
class Vineyard_persistence
{
public:
    Vineyard_persistence();
    Vineyard_persistence(
            const boundary_matrix& matrix,
            bool verbose = false);
    Vineyard_persistence(
            const boundary_matrix& matrix,
            const filtration_type& filter,
            bool verbose = false);
    Vineyard_persistence(
            const Vineyard_matrix_type& matrix,
            bool verbose = false);
    Vineyard_persistence(
            const Vineyard_matrix_type& matrix,
            const filtration_type& filter,
            bool verbose = false);
    Vineyard_persistence(
            const Vineyard_matrix_type& matrix,
            const filtration_type& filter,
            const permutation_type& permutation,
            bool verbose = false);

    void initialize_barcode();
    void update(filtration_type &newFilter);
    const diagram_type &get_diagram();
    void display_diagram();
    void display_filtration();

    Vineyard_persistence& operator=(Vineyard_persistence other);

private:
    Vineyard_matrix_type matrix_;
    permutation_type currentToOriginalPositions_;  //new pos to origin pos
    diagram_type diagram_;
    filtration_type filter_;
    const bool verbose_;

    filtration_value_type _get_current_filtration_value(index index);
    void _sort_matrix(const boundary_matrix& matrix);
    void _initialize_permutations();
    void _initialize_filter();
//     void _update_old(filtration_type &newFilter);
};

template<class Vineyard_matrix_type>
inline Vineyard_persistence<Vineyard_matrix_type>::Vineyard_persistence()
{}

template<class Vineyard_matrix_type>
inline Vineyard_persistence<Vineyard_matrix_type>::Vineyard_persistence(
        const boundary_matrix& matrix,
        bool verbose)
    : matrix_(matrix),
      currentToOriginalPositions_(matrix.size()),
      filter_(matrix.size()),
      verbose_(verbose)
{   Debug::Timer timer("Creating matrix... ",verbose_);
//     if (verbose_) std::cout << "Creating matrix ..." << std::flush;

    _initialize_filter();
    _sort_matrix(matrix);

    if constexpr (Debug::debug){
        Debug::disp_vect(filter_);
        matrix_.print_matrices();
    }

//     if (verbose_) std::cout << " Done !" << std::endl;
}

//assumes filter[i] corresponds to filtration value of matrix[i]
template<class Vineyard_matrix_type>
inline Vineyard_persistence<Vineyard_matrix_type>::Vineyard_persistence(
        const boundary_matrix& matrix,
        const filtration_type& filter,
        bool verbose)
    : currentToOriginalPositions_(matrix.size()),
      filter_(filter),
      verbose_(verbose)
{   Debug::Timer timer("Creating matrix... ",verbose_);
//     if (verbose_) std::cout << "Creating matrix ..." << std::flush;

    _sort_matrix(matrix);

    if constexpr (Debug::debug){
        Debug::disp_vect(filter_);
        matrix_.print_matrices();
    }

//     if (verbose_) std::cout << " Done !" << std::endl;
}

//copy, i.e. assumes matrix sorted
template<class Vineyard_matrix_type>
inline Vineyard_persistence<Vineyard_matrix_type>::Vineyard_persistence(
        const Vineyard_matrix_type& matrix,
        bool verbose)
    : matrix_(matrix),
      currentToOriginalPositions_(matrix.size()),
      filter_(matrix.size()),
      verbose_(verbose)
{   Debug::Timer timer("Creating matrix... ",verbose_);
//     if (verbose_) std::cout << "Creating matrix ..." << std::flush;

    _initialize_permutations();
    _initialize_filter();

    if constexpr (Debug::debug){
        Debug::disp_vect(filter_);
        matrix_.print_matrices();
    }

//     if (verbose_) std::cout << " Done !" << std::endl;
}

//copy, i.e. assumes matrix sorted and filter[i] corresponds to
//filtration value of matrix[i]
template<class Vineyard_matrix_type>
inline Vineyard_persistence<Vineyard_matrix_type>::Vineyard_persistence(
        const Vineyard_matrix_type& matrix,
        const filtration_type& filter,
        bool verbose)
    : matrix_(matrix),
      currentToOriginalPositions_(matrix.size()),
      filter_(filter),
      verbose_(verbose)
{   Debug::Timer timer("Creating matrix... ",verbose_);
//     if (verbose_) std::cout << "Creating matrix ..." << std::flush;

    _initialize_permutations();

    if constexpr (Debug::debug){
        Debug::disp_vect(filter_);
        matrix_.print_matrices();
    }

//     if (verbose_) std::cout << " Done !" << std::endl;
}

//copy, i.e. assumes matrix sorted and filter[permutation[i]]
//corresponds to filtration value of matrix[i]
template<class Vineyard_matrix_type>
inline Vineyard_persistence<Vineyard_matrix_type>::Vineyard_persistence(
        const Vineyard_matrix_type& matrix,
        const filtration_type& filter,
        const permutation_type& permutation,    //new to original
        bool verbose)
    : matrix_(matrix),
      currentToOriginalPositions_(permutation),
      filter_(filter),
      verbose_(verbose)
{   Debug::Timer timer("Creating matrix",verbose_);
//     if (verbose_) std::cout << "Creating matrix ..." << std::flush;

    if constexpr (Debug::debug){
        Debug::disp_vect(filter_);
        matrix_.print_matrices();
    }

//     if (verbose_) std::cout << " Done !" << std::endl;
}

/**
 * @brief Computes the first barcode, and fills the variables in the class.
 * @param verbose
 * @param debug
 */
template<class Vineyard_matrix_type>
inline void Vineyard_persistence<Vineyard_matrix_type>::initialize_barcode()
{
    if (verbose_) {
        std::cout << "Initializing barcode";
    }
    auto elapsed = clock();

    matrix_.initialize();

    if (verbose_)
    {
        elapsed = clock() - elapsed;
        std::cout << "... Done ! It took " << ((float)elapsed) / CLOCKS_PER_SEC
                  << " seconds." << std::endl;
    }
}

// template<class Vineyard_matrix_type>
// inline void Vineyard_persistence<Vineyard_matrix_type>::update(
//         filtration_type &newFilter)
// {
//     _update_old(newFilter);
// }

/**
 * @brief Gets diagram from RU decomposition.
 */
template<class Vineyard_matrix_type>
inline const diagram_type& Vineyard_persistence<Vineyard_matrix_type>::get_diagram()
{
    const barcode_type& barcode = matrix_.get_current_barcode();
    unsigned int size = barcode.size();

    diagram_.resize(size);

    for (unsigned int i = 0; i < size; i++){
        const Bar& bar = barcode.at(i);
        Diagram_point& point = diagram_.at(i);
        point.dim = bar.dim;
        point.birth = _get_current_filtration_value(bar.birth);
		point.death = (bar.death == -1 ?
						   inf
						 : _get_current_filtration_value(bar.death));
	}

    return diagram_;
}

template<class Vineyard_matrix_type>
inline void Vineyard_persistence<Vineyard_matrix_type>::display_diagram()
{
    for (const Diagram_point& point : diagram_){
        std::cout << point.dim << " " << point.birth << " "
                  << point.death << std::endl;
    }
}

template<class Vineyard_matrix_type>
inline void Vineyard_persistence<Vineyard_matrix_type>::display_filtration()
{
    std::cout << "filter:\n";
    for (double filtValue : filter_)
        std::cout << filtValue << " ";
    std::cout << std::endl;
}

template<class Vineyard_matrix_type>
inline Vineyard_persistence<Vineyard_matrix_type>&
Vineyard_persistence<Vineyard_matrix_type>::operator=(
        Vineyard_persistence<Vineyard_matrix_type> other)
{
    std::swap(matrix_, other.matrix_);
    std::swap(currentToOriginalPositions_, other.currentToOriginalPositions_);
    std::swap(diagram_, other.diagram_);
    std::swap(filter_, other.filter_);
    return *this;
}

template<class Vineyard_matrix_type>
inline filtration_value_type
Vineyard_persistence<Vineyard_matrix_type>::_get_current_filtration_value(
        index index)
{
	return filter_.at(currentToOriginalPositions_.at(index));
}

template<class Vineyard_matrix_type>
inline void Vineyard_persistence<Vineyard_matrix_type>::_sort_matrix(
        const boundary_matrix &matrix)
{
//     auto is_strict_less_than = [&matrix, this](
//             unsigned int index1, unsigned int index2)
//     {
//         if (matrix.at(index1).size() != matrix.at(index2).size())
//             return matrix.at(index1).size() < matrix.at(index2).size();
//         return filter_.at(index1) < filter_.at(index2);
//     };
    auto is_less_filtration = [this]( // Dimension is already assumed to be sorted
            unsigned int index1, unsigned int index2)
    {
        return filter_.at(index1) < filter_.at(index2);
    };

    permutation_type permutationInv(currentToOriginalPositions_.size());
    _initialize_permutations();
//     {Debug::Timer timer("Sorting matrix ...", verbose_);
//         std::sort(currentToOriginalPositions_.begin(), currentToOriginalPositions_.end(), is_strict_less_than);
        unsigned int iterator = 1;
        while(iterator < matrix.size() ){
            auto first = iterator-1;
            while(iterator < matrix.size() && matrix.at(iterator).size() ==  matrix.at(iterator-1).size()){
                iterator++;
            }
            std::sort(currentToOriginalPositions_.begin()+first , currentToOriginalPositions_.begin() + iterator, is_less_filtration );
            iterator++;
        }
//     }
//     {Debug::Timer timer("Initialisation permutation ...", verbose_);
        for (unsigned int i = 0; i < permutationInv.size(); i++)
        permutationInv.at(currentToOriginalPositions_.at(i)) = i;
//     }
    boundary_matrix sorted(matrix.size());
//     {Debug::Timer timer("Transfering matrix ...", verbose_);
        for (unsigned int i = 0; i < matrix.size(); i++){
            boundary_type& b = sorted.at(permutationInv.at(i));
            b = matrix.at(i);
            for (index& id : b)
                id = permutationInv.at(id);
            std::sort(b.begin(), b.end());
        }
//     }

    matrix_ = Vineyard_matrix_type(sorted);
}

template<class Vineyard_matrix_type>
inline void Vineyard_persistence<Vineyard_matrix_type>::_initialize_permutations()
{
    for (unsigned int i = 0; i < currentToOriginalPositions_.size(); i++)
        currentToOriginalPositions_.at(i) = i;
}

template<class Vineyard_matrix_type>
inline void Vineyard_persistence<Vineyard_matrix_type>::_initialize_filter()
{
    for (unsigned int i = 0; i < filter_.size(); i++)
        filter_.at(i) = i;
}

template<class Vineyard_matrix_type>
inline void Vineyard_persistence<Vineyard_matrix_type>::update(
        filtration_type &newFilter)
{
    int n = matrix_.get_number_of_simplices();
    filter_.swap(newFilter);

//    uint k = 0;

    bool sorted = false;
    for (int i = n - 1; i > 0 && !sorted; i--)
    {
        sorted = true;
        for (int j = 0; j < i; j++)
        {
            if (matrix_.get_dimension(j) == matrix_.get_dimension(j + 1) &&
                    _get_current_filtration_value(j) > _get_current_filtration_value(j + 1))
            {
                matrix_.vine_swap(j);
                std::swap(currentToOriginalPositions_[j],
                          currentToOriginalPositions_[j + 1]);
                sorted = false;
//                if (verbose_) k++;
            }
        }
    }
//    if (verbose_)
//        std::cout << "Permuted " << k << "times, with " << n
//                  << " simplices." << std::endl;
};

}   //namespace Vineyard

#endif // VINEYARDS_H_INCLUDED
