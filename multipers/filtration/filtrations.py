import gudhi as gd
import numpy as np
from math import sqrt
from scipy.spatial import KDTree
from scipy.spatial.distance import cdist
from numpy.typing import ArrayLike
from typing import Optional
from multipers.ml.convolutions import available_kernels, KDE, DTM

import multipers as mp
import multipers.slicer as mps


def RipsLowerstar( *, 
        points:Optional[ArrayLike] = None,
        distance_matrix:Optional[ArrayLike]=None,
        function=None,
        threshold_radius=None
    ):
    """
    Computes the Rips complex, with the usual rips filtration as a first parameter,
    and the lower star multi filtration as other parameter.

    Input:
     - points or distance_matrix: ArrayLike
     - function : ArrayLike of shape (num_data, num_parameters -1)
     - threshold_radius:  max edge length of the rips. Defaults at min(max(distance_matrix, axis=1)).
    """
    assert points is not None or distance_matrix is not None, "`points` or `distance_matrix` has to be given."
    if distance_matrix is None:
        distance_matrix = cdist(points, points) # this may be slow...
    if threshold_radius is None:
        threshold_radius = np.min(np.max(distance_matrix, axis=1))
    st = gd.RipsComplex(distance_matrix = distance_matrix, max_edge_length=threshold_radius).create_simplex_tree()
    if function is None:
        return mp.SimplexTreeMulti(st, num_parameters = 1)

    function = np.asarray(function)
    if function.ndim == 1:
        function = function[:,None]
    num_parameters = function.shape[1] +1 
    st = mp.SimplexTreeMulti(st, num_parameters = num_parameters)
    for i in range(function.shape[1]):
        st.fill_lowerstar(function[:,i], parameter = 1+i)
    return st

def RipsCodensity(
    points: ArrayLike,
    bandwidth: Optional[float] = None,
    *,
    return_log: bool = True,
    dtm_mass:Optional[float]=None,
    kernel: available_kernels="gaussian",
    threshold_radius: Optional[float] = None,
):
    """
    Computes the Rips density filtration.
    """
    assert bandwidth is None or dtm_mass is None, "Density estimation is either via kernels or dtm."
    if bandwidth is not None:
        kde = KDE(bandwidth=bandwidth, kernel=kernel, return_log=return_log)
        f = kde.fit(points).score_samples(points)
    elif dtm_mass is not None:
        f = DTM(masses=[dtm_mass]).fit(points).score_samples(points)[0]
    else:
        raise ValueError("Bandwidth or DTM mass has to be given.")
    return RipsLowerstar(points=points, function=f, threshold_radius=threshold_radius)


def DelaunayLowerstar(
        points:ArrayLike,
        function:ArrayLike,
        *,
        distance_matrix:Optional[ArrayLike]=None,
        threshold_radius:Optional[float]=None,
        reduce_degree:int=-1, 
        vineyard:Optional[bool]=None, 
        dtype=np.float64, 
        verbose:bool=False, 
        clear:bool=True 
    ):
    """
    Computes the Function Delaunay bifiltration. Similar to RipsLowerstar, but most suited for low-dimensional euclidean data.

    Input:
     - points or distance_matrix: ArrayLike
     - function : ArrayLike of shape (num_data, ) 
     - threshold_radius:  max edge length of the rips. Defaults at min(max(distance_matrix, axis=1)).
    """
    assert distance_matrix is None, "Delaunay cannot be built from distance matrices"
    if threshold_radius is not None:
        raise NotImplementedError("Delaunay with threshold not implemented yet.")
    points = np.asarray(points)
    function = np.asarray(function).squeeze()
    assert function.ndim == 1, "Delaunay Lowerstar is only compatible with 1 additional parameter." 
    return mps.from_function_delaunay(points, function, degree=reduce_degree, vineyard=vineyard, dtype=dtype, verbose=verbose, clear=clear)

def DelaunayCodensity(
    points: ArrayLike,
    bandwidth: Optional[float] = None,
    *,
    return_log: bool = True,
    dtm_mass:Optional[float]=None,
    kernel: available_kernels="gaussian",
    threshold_radius:Optional[float]=None,
    reduce_degree:int=-1, 
    vineyard:Optional[bool]=None, 
    dtype=np.float64, 
    verbose:bool=False, 
    clear:bool=True 
):
    """
    TODO
    """
    assert bandwidth is None or dtm_mass is None, "Density estimation is either via kernels or dtm."
    if bandwidth is not None:
        kde = KDE(bandwidth=bandwidth, kernel=kernel, return_log=return_log)
        f = kde.fit(points).score_samples(points)
    elif dtm_mass is not None:
        f = DTM(masses=[dtm_mass]).fit(points).score_samples(points)[0]
    else:
        raise ValueError("Bandwidth or DTM mass has to be given.")
    return DelaunayLowerstar(
        points=points,
        function=f,
        threshold_radius=threshold_radius,
        reduce_degree=reduce_degree,
        vineyard=vineyard,
        dtype=dtype,
        verbose=verbose,
        clear=clear,
    )

def Cubical(image:ArrayLike, **slicer_kwargs):
    """
    Computes the cubical filtration of an image. 
    The last axis dimention is interpreted as the number of parameters.

    Input:
     - image: ArrayLike of shape (*image_resolution, num_parameters)
     - ** args : specify non-default slicer parameters
    """
    return mps.from_bitmap(image, **slicer_kwargs)

def DegreeRips(*, points = None, distance_matrix=None, ks=None, threshold_radius=None):
    """
    The DegreeRips filtration.
    """

    raise NotImplementedError("Use the default implentation ftm.")

def CoreDelaunay(
    points: ArrayLike,
    *,
    beta: float = 1.0,
    k_max: Optional[int] = None,
    k_step: int = 1,
    precision: str = "safe",
    verbose: bool = False,
    max_alpha_square: float = float("inf"),
) -> mp.simplex_tree_multi.SimplexTreeMulti_KFf64:
    """
    Computes the Delaunay core bifiltration of a point cloud presented in the paper "Core Bifiltration" https://arxiv.org/abs/2405.01214, and returns the (multi-critical) bifiltration as a SimplexTreeMulti. The Delaunay core bifiltration is an alpha complex version of the core bifiltration which is smaller in size. Moreover, along the horizontal line k=1, the Delaunay core bifiltration is identical to the alpha complex.

    Input:
     - points: The point cloud as an ArrayLike of shape (n, d) where n is the number of points and d is the dimension of the points.
     - beta: The beta parameter for the Delaunay Core Bifiltration (default 1.0).
     - k_max: The maximum number of nearest neighbors to consider (default None). If None or greater than the number of points in the point cloud, k_max is set to the number of points in the point cloud.
     - k_step: The step size for the number of nearest neighbors (default 1).
     - precision: The precision of the computation of the AlphaComplex, one of ['safe', 'exact', 'fast'] (default 'safe'). See the GUDHI documentation for more information.
     - verbose: Whether to print progress messages (default False).
     - max_alpha_square: The maximum squared alpha value to consider when createing the alpha complex (default inf). See the GUDHI documentation for more information.
    """
    if k_max is None or k_max > len(points):
        k_max = len(points)

    assert len(points) > 0, f"The point cloud must contain at least one point."
    assert points.ndim == 2, f"The point cloud must be a 2D array, got {points.ndim}D."
    assert beta >= 0, f"The parameter beta must be positive, got {beta}."
    assert k_max > 0, f"The parameter k_max must be positive, got {k_max}."
    assert k_step > 0, f"The parameter k_step must be positive, got {k_step}."
    assert precision in ["safe", "exact", "fast"], (
        f"The parameter precision must be one of ['safe', 'exact', 'fast'], got {precision}."
    )
    ks = np.arange(1, k_max + 1, k_step)
    if verbose:
        print(
            f"Computing the Delaunay Core Bifiltration of {len(points)} points in dimension {points.shape[1]} with parameters:"
        )
        print(f"\tbeta = {beta}")
        print(f"\tk_max = {k_max}")
        print(f"\tk_step = {k_step} (total of {len(ks)} k-values)")

    if verbose:
        print("Building the alpha complex...")
    alpha_complex = gd.AlphaComplex(
        points=points, precision=precision
    ).create_simplex_tree(max_alpha_square=max_alpha_square)

    if verbose:
        print("Computing the k-nearest neighbor distances...")
    knn_distances = KDTree(points).query(points, k=ks)[0]

    max_dim = alpha_complex.dimension()
    vertex_arrays_in_dimension = [[] for _ in range(max_dim + 1)]
    squared_alphas_in_dimension = [[] for _ in range(max_dim + 1)]
    for simplex, alpha_squared in alpha_complex.get_simplices():
        dim = len(simplex) - 1
        squared_alphas_in_dimension[dim].append(alpha_squared)
        vertex_arrays_in_dimension[dim].append(simplex)

    alphas_in_dimension = [np.sqrt(np.array(alpha_squared, dtype=np.float64)) for alpha_squared in squared_alphas_in_dimension]
    vertex_arrays_in_dimension = [np.array(vertex_array, dtype=np.int32) for vertex_array in vertex_arrays_in_dimension]

    simplex_tree_multi = mp.SimplexTreeMulti(
        num_parameters=2, kcritical=True, dtype=np.float64
    )

    for dim, (vertex_array, alphas) in enumerate(zip(vertex_arrays_in_dimension, alphas_in_dimension)):
        num_simplices = len(vertex_array)
        if verbose:
            print(
                f"Inserting {num_simplices} simplices of dimension {dim} ({num_simplices * len(ks)} birth values)..."
            )
        max_knn_distances = np.max(knn_distances[vertex_array], axis=1)
        critical_radii = np.maximum(alphas[:, None], beta * max_knn_distances)
        filtrations = np.stack((critical_radii, -ks * np.ones_like(critical_radii)), axis=-1)
        simplex_tree_multi.insert_batch(vertex_array.T, filtrations)

    if verbose:
        print("Done computing the Delaunay Core Bifiltration.")

    return simplex_tree_multi

